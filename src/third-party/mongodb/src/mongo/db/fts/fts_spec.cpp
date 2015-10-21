// fts_spec.cpp
/**
*    Copyright (C) 2012 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*    As a special exception, the copyright holders give permission to link the
*    code of portions of this program with the OpenSSL library under certain
*    conditions as described in each individual source file and distribute
*    linked combinations including the program with the OpenSSL library. You
*    must comply with the GNU Affero General Public License in all respects for
*    all of the code used other than as permitted herein. If you modify file(s)
*    with this exception, you may extend this exception to your version of the
*    file(s), but you are not obligated to do so. If you do not wish to do so,
*    delete this exception statement from your version. If you delete this
*    exception statement from all source files in the program, then also delete
*    it in the license file.
*/

#include "mongo/platform/basic.h"

#include "mongo/db/fts/fts_spec.h"

#include "mongo/db/field_ref.h"
#include "mongo/db/fts/fts_element_iterator.h"
#include "mongo/db/fts/fts_util.h"
#include "mongo/util/mongoutils/str.h"
#include "mongo/util/stringutils.h"

namespace mongo {

namespace fts {

using std::map;
using std::string;
using namespace mongoutils;

const double DEFAULT_WEIGHT = 1;
const double MAX_WEIGHT = 1000000000;
const double MAX_WORD_WEIGHT = MAX_WEIGHT / 10000;

namespace {
// Default language.  Used for new indexes.
const std::string moduleDefaultLanguage("english");

/** Validate the given language override string. */
bool validateOverride(const string& override) {
    // The override field can't be empty, can't be prefixed with a dollar sign, and
    // can't contain a dot.
    return !override.empty() && override[0] != '$' && override.find('.') == std::string::npos;
}
}

FTSSpec::FTSSpec(const BSONObj& indexInfo) {
    // indexInfo is a text index spec.  Text index specs pass through fixSpec() before
    // being saved to the system.indexes collection.  fixSpec() enforces a schema, such that
    // required fields must exist and be of the correct type (e.g. weights,
    // textIndexVersion).
    massert(16739, "found invalid spec for text index", indexInfo["weights"].isABSONObj());
    BSONElement textIndexVersionElt = indexInfo["textIndexVersion"];
    massert(17367,
            "found invalid spec for text index, expected number for textIndexVersion",
            textIndexVersionElt.isNumber());

    // We currently support TEXT_INDEX_VERSION_1 (deprecated) and TEXT_INDEX_VERSION_2.
    // Reject all other values.
    massert(17364,
            str::stream() << "attempt to use unsupported textIndexVersion "
                          << textIndexVersionElt.numberInt() << "; versions supported: "
                          << TEXT_INDEX_VERSION_2 << ", " << TEXT_INDEX_VERSION_1,
            textIndexVersionElt.numberInt() == TEXT_INDEX_VERSION_2 ||
                textIndexVersionElt.numberInt() == TEXT_INDEX_VERSION_1);

    _textIndexVersion = (textIndexVersionElt.numberInt() == TEXT_INDEX_VERSION_2)
        ? TEXT_INDEX_VERSION_2
        : TEXT_INDEX_VERSION_1;

    // Initialize _defaultLanguage.  Note that the FTSLanguage constructor requires
    // textIndexVersion, since language parsing is version-specific.
    StatusWithFTSLanguage swl =
        FTSLanguage::make(indexInfo["default_language"].String(), _textIndexVersion);
    verify(swl.getStatus().isOK());  // should not fail, since validated by fixSpec().
    _defaultLanguage = swl.getValue();

    _languageOverrideField = indexInfo["language_override"].valuestrsafe();

    _wildcard = false;

    // in this block we fill in the _weights map
    {
        BSONObjIterator i(indexInfo["weights"].Obj());
        while (i.more()) {
            BSONElement e = i.next();
            verify(e.isNumber());

            if (WILDCARD == e.fieldName()) {
                _wildcard = true;
            } else {
                double num = e.number();
                _weights[e.fieldName()] = num;
                verify(num > 0 && num < MAX_WORD_WEIGHT);
            }
        }
        verify(_wildcard || _weights.size());
    }

    // extra information
    {
        BSONObj keyPattern = indexInfo["key"].Obj();
        verify(keyPattern.nFields() >= 2);
        BSONObjIterator i(keyPattern);

        bool passedFTS = false;

        while (i.more()) {
            BSONElement e = i.next();
            if (str::equals(e.fieldName(), "_fts") || str::equals(e.fieldName(), "_ftsx")) {
                passedFTS = true;
                continue;
            }

            if (passedFTS)
                _extraAfter.push_back(e.fieldName());
            else
                _extraBefore.push_back(e.fieldName());
        }
    }
}

const FTSLanguage* FTSSpec::_getLanguageToUseV2(const BSONObj& userDoc,
                                                const FTSLanguage* currentLanguage) const {
    BSONElement e = userDoc[_languageOverrideField];
    if (e.eoo()) {
        return currentLanguage;
    }
    uassert(17261,
            "found language override field in document with non-string type",
            e.type() == mongo::String);
    StatusWithFTSLanguage swl = FTSLanguage::make(e.String(), TEXT_INDEX_VERSION_2);
    uassert(17262, "language override unsupported: " + e.String(), swl.getStatus().isOK());
    return swl.getValue();
}

void FTSSpec::scoreDocument(const BSONObj& obj, TermFrequencyMap* term_freqs) const {
    if (_textIndexVersion == TEXT_INDEX_VERSION_1) {
        return _scoreDocumentV1(obj, term_freqs);
    }

    FTSElementIterator it(*this, obj);

    while (it.more()) {
        FTSIteratorValue val = it.next();
        Stemmer stemmer(*val._language);
        Tools tools(*val._language, &stemmer, StopWords::getStopWords(*val._language));
        _scoreStringV2(tools, val._text, term_freqs, val._weight);
    }
}

void FTSSpec::_scoreStringV2(const Tools& tools,
                             const StringData& raw,
                             TermFrequencyMap* docScores,
                             double weight) const {
    ScoreHelperMap terms;

    unsigned numTokens = 0;

    Tokenizer i(tools.language, raw);
    while (i.more()) {
        Token t = i.next();
        if (t.type != Token::TEXT)
            continue;

        string term = t.data.toString();
        makeLower(&term);
        if (tools.stopwords->isStopWord(term)) {
            continue;
        }
        term = tools.stemmer->stem(term);

        ScoreHelperStruct& data = terms[term];

        if (data.exp) {
            data.exp *= 2;
        } else {
            data.exp = 1;
        }
        data.count += 1;
        data.freq += (1 / data.exp);
        numTokens++;
    }

    for (ScoreHelperMap::const_iterator i = terms.begin(); i != terms.end(); ++i) {
        const string& term = i->first;
        const ScoreHelperStruct& data = i->second;

        // in order to adjust weights as a function of term count as it
        // relates to total field length. ie. is this the only word or
        // a frequently occuring term? or does it only show up once in
        // a long block of text?

        double coeff = (0.5 * data.count / numTokens) + 0.5;

        // if term is identical to the raw form of the
        // field (untokenized) give it a small boost.
        double adjustment = 1;
        if (raw.size() == term.length() && raw.equalCaseInsensitive(term))
            adjustment += 0.1;

        double& score = (*docScores)[term];
        score += (weight * data.freq * coeff * adjustment);
        verify(score <= MAX_WEIGHT);
    }
}

Status FTSSpec::getIndexPrefix(const BSONObj& query, BSONObj* out) const {
    if (numExtraBefore() == 0) {
        *out = BSONObj();
        return Status::OK();
    }

    BSONObjBuilder b;
    for (unsigned i = 0; i < numExtraBefore(); i++) {
        BSONElement e = query.getFieldDotted(extraBefore(i));
        if (e.eoo())
            return Status(ErrorCodes::BadValue,
                          str::stream() << "need have an equality filter on: " << extraBefore(i));

        if (e.isABSONObj() && e.Obj().firstElement().getGtLtOp(-1) != -1)
            return Status(ErrorCodes::BadValue,
                          str::stream() << "need have an equality filter on: " << extraBefore(i));

        b.append(e);
    }
    *out = b.obj();
    return Status::OK();
}

namespace {
void _addFTSStuff(BSONObjBuilder* b) {
    b->append("_fts", INDEX_NAME);
    b->append("_ftsx", 1);
}

void verifyFieldNameNotReserved(StringData s) {
    uassert(17289,
            "text index with reserved fields _fts/_ftsx not allowed",
            s != "_fts" && s != "_ftsx");
}
}

BSONObj FTSSpec::fixSpec(const BSONObj& spec) {
    if (spec["textIndexVersion"].numberInt() == TEXT_INDEX_VERSION_1) {
        return _fixSpecV1(spec);
    }

    map<string, int> m;

    BSONObj keyPattern;
    {
        BSONObjBuilder b;

        // Populate m and keyPattern.
        {
            bool addedFtsStuff = false;
            BSONObjIterator i(spec["key"].Obj());
            while (i.more()) {
                BSONElement e = i.next();
                if (str::equals(e.fieldName(), "_fts")) {
                    uassert(17271, "expecting _fts:\"text\"", INDEX_NAME == e.valuestrsafe());
                    addedFtsStuff = true;
                    b.append(e);
                } else if (str::equals(e.fieldName(), "_ftsx")) {
                    uassert(17272, "expecting _ftsx:1", e.numberInt() == 1);
                    b.append(e);
                } else if (e.type() == String && INDEX_NAME == e.valuestr()) {
                    if (!addedFtsStuff) {
                        _addFTSStuff(&b);
                        addedFtsStuff = true;
                    }

                    m[e.fieldName()] = 1;
                } else {
                    uassert(17273,
                            "expected value 1 or -1 for non-text key in compound index",
                            e.numberInt() == 1 || e.numberInt() == -1);
                    b.append(e);
                }
            }
            verify(addedFtsStuff);
        }
        keyPattern = b.obj();

        // Verify that index key is in the correct format: extraBefore fields, then text
        // fields, then extraAfter fields.
        {
            BSONObjIterator i(spec["key"].Obj());
            verify(i.more());
            BSONElement e = i.next();

            // extraBefore fields
            while (String != e.type()) {
                verifyFieldNameNotReserved(e.fieldNameStringData());
                verify(i.more());
                e = i.next();
            }

            // text fields
            bool alreadyFixed = str::equals(e.fieldName(), "_fts");
            if (alreadyFixed) {
                uassert(17288, "expected _ftsx after _fts", i.more());
                e = i.next();
                uassert(17274, "expected _ftsx after _fts", str::equals(e.fieldName(), "_ftsx"));
                e = i.next();
            } else {
                do {
                    verifyFieldNameNotReserved(e.fieldNameStringData());
                    e = i.next();
                } while (!e.eoo() && e.type() == String);
            }

            // extraAfterFields
            while (!e.eoo()) {
                uassert(17389, "'text' fields in index must all be adjacent", e.type() != String);
                verifyFieldNameNotReserved(e.fieldNameStringData());
                e = i.next();
            }
        }
    }

    if (spec["weights"].type() == Object) {
        BSONObjIterator i(spec["weights"].Obj());
        while (i.more()) {
            BSONElement e = i.next();
            uassert(17283, "weight for text index needs numeric type", e.isNumber());
            m[e.fieldName()] = e.numberInt();
        }
    } else if (spec["weights"].str() == WILDCARD) {
        m[WILDCARD] = 1;
    } else if (!spec["weights"].eoo()) {
        uasserted(17284, "text index option 'weights' must be an object");
    }

    BSONObj weights;
    {
        BSONObjBuilder b;
        for (map<string, int>::iterator i = m.begin(); i != m.end(); ++i) {
            uassert(16674, "score for word too high", i->second > 0 && i->second < MAX_WORD_WEIGHT);

            // Verify weight refers to a valid field.
            if (i->first != "$**") {
                FieldRef keyField(i->first);
                uassert(17294, "weight cannot be on an empty field", keyField.numParts() != 0);
                for (size_t partNum = 0; partNum < keyField.numParts(); partNum++) {
                    StringData part = keyField.getPart(partNum);
                    uassert(17291, "weight cannot have empty path component", !part.empty());
                    uassert(17292,
                            "weight cannot have path component with $ prefix",
                            !part.startsWith("$"));
                }
            }

            b.append(i->first, i->second);
        }
        weights = b.obj();
    }

    BSONElement default_language_elt = spec["default_language"];
    string default_language(default_language_elt.str());
    if (default_language_elt.eoo()) {
        default_language = moduleDefaultLanguage;
    } else {
        uassert(
            17263, "default_language needs a string type", default_language_elt.type() == String);
    }
    uassert(17264,
            "default_language is not valid",
            FTSLanguage::make(default_language, TEXT_INDEX_VERSION_2).getStatus().isOK());

    BSONElement language_override_elt = spec["language_override"];
    string language_override(language_override_elt.str());
    if (language_override_elt.eoo()) {
        language_override = "language";
    } else {
        uassert(17136,
                "language_override is not valid",
                language_override_elt.type() == String && validateOverride(language_override));
    }

    int version = -1;
    int textIndexVersion = TEXT_INDEX_VERSION_2;

    BSONObjBuilder b;
    BSONObjIterator i(spec);
    while (i.more()) {
        BSONElement e = i.next();
        if (str::equals(e.fieldName(), "key")) {
            b.append("key", keyPattern);
        } else if (str::equals(e.fieldName(), "weights")) {
            b.append("weights", weights);
            weights = BSONObj();
        } else if (str::equals(e.fieldName(), "default_language")) {
            b.append("default_language", default_language);
            default_language = "";
        } else if (str::equals(e.fieldName(), "language_override")) {
            b.append("language_override", language_override);
            language_override = "";
        } else if (str::equals(e.fieldName(), "v")) {
            version = e.numberInt();
        } else if (str::equals(e.fieldName(), "textIndexVersion")) {
            uassert(17293, "text index option 'textIndexVersion' must be a number", e.isNumber());
            textIndexVersion = e.numberInt();
            uassert(16730,
                    str::stream() << "bad textIndexVersion: " << textIndexVersion,
                    textIndexVersion == TEXT_INDEX_VERSION_2);
        } else {
            b.append(e);
        }
    }

    if (!weights.isEmpty()) {
        b.append("weights", weights);
    }
    if (!default_language.empty()) {
        b.append("default_language", default_language);
    }
    if (!language_override.empty()) {
        b.append("language_override", language_override);
    }
    if (version >= 0) {
        b.append("v", version);
    }
    b.append("textIndexVersion", textIndexVersion);

    return b.obj();
}
}
}
