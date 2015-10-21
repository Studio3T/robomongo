#include "emitterstate.h"
#include "yaml-cpp/exceptions.h"
#include <limits>

namespace YAML
{
	EmitterState::EmitterState(): m_isGood(true), m_curIndent(0), m_hasAnchor(false), m_hasTag(false), m_hasNonContent(false), m_docCount(0)
	{
		// set default global manipulators
		m_charset.set(EmitNonAscii);
		m_strFmt.set(Auto);
		m_boolFmt.set(TrueFalseBool);
		m_boolLengthFmt.set(LongBool);
		m_boolCaseFmt.set(LowerCase);
		m_intFmt.set(Dec);
		m_indent.set(2);
		m_preCommentIndent.set(2);
		m_postCommentIndent.set(1);
		m_seqFmt.set(Block);
		m_mapFmt.set(Block);
		m_mapKeyFmt.set(Auto);
        m_floatPrecision.set(std::numeric_limits<float>::digits10 + 1);
        m_doublePrecision.set(std::numeric_limits<double>::digits10 + 1);
	}
	
	EmitterState::~EmitterState()
	{
	}

	// SetLocalValue
	// . We blindly tries to set all possible formatters to this value
	// . Only the ones that make sense will be accepted
	void EmitterState::SetLocalValue(EMITTER_MANIP value)
	{
		SetOutputCharset(value, FmtScope::Local);
		SetStringFormat(value, FmtScope::Local);
		SetBoolFormat(value, FmtScope::Local);
		SetBoolCaseFormat(value, FmtScope::Local);
		SetBoolLengthFormat(value, FmtScope::Local);
		SetIntFormat(value, FmtScope::Local);
		SetFlowType(GroupType::Seq, value, FmtScope::Local);
		SetFlowType(GroupType::Map, value, FmtScope::Local);
		SetMapKeyFormat(value, FmtScope::Local);
	}
	
    void EmitterState::SetAnchor()
    {
        m_hasAnchor = true;
    }
        
    void EmitterState::SetTag()
    {
        m_hasTag = true;
    }

    void EmitterState::SetNonContent()
    {
        m_hasNonContent = true;
    }

    void EmitterState::SetLongKey()
    {
        assert(!m_groups.empty());        
        if(m_groups.empty())
            return;
        
        assert(m_groups.top().type == GroupType::Map);
        m_groups.top().longKey = true;
    }
    
    void EmitterState::ForceFlow()
    {
        assert(!m_groups.empty());
        if(m_groups.empty())
            return;
        
        m_groups.top().flowType = FlowType::Flow;
    }

    void EmitterState::StartedNode()
    {
        if(m_groups.empty()) {
            m_docCount++;
        } else {
            m_groups.top().childCount++;
            if(m_groups.top().childCount % 2 == 0)
                m_groups.top().longKey = false;
        }
        
        m_hasAnchor = false;
        m_hasTag = false;
        m_hasNonContent = false;
    }

    EmitterNodeType::value EmitterState::NextGroupType(GroupType::value type) const
    {
        if(type == GroupType::Seq) {
            if(GetFlowType(type) == Block)
                return EmitterNodeType::BlockSeq;
            else
                return EmitterNodeType::FlowSeq;
        } else {
            if(GetFlowType(type) == Block)
                return EmitterNodeType::BlockMap;
            else
                return EmitterNodeType::FlowMap;
        }
        
        // can't happen
        assert(false);
        return EmitterNodeType::None;
    }
    
    void EmitterState::StartedDoc()
    {
        m_hasAnchor = false;
        m_hasTag = false;
        m_hasNonContent = false;
    }
    
    void EmitterState::EndedDoc()
    {
        m_hasAnchor = false;
        m_hasTag = false;
        m_hasNonContent = false;
    }

    void EmitterState::StartedScalar()
    {
        StartedNode();
        ClearModifiedSettings();
    }

	void EmitterState::StartedGroup(GroupType::value type)
	{
        StartedNode();

		const int lastGroupIndent = (m_groups.empty() ? 0 : m_groups.top().indent);
		m_curIndent += lastGroupIndent;
		
		std::auto_ptr<Group> pGroup(new Group(type));
		
		// transfer settings (which last until this group is done)
		pGroup->modifiedSettings = m_modifiedSettings;

		// set up group
        if(GetFlowType(type) == Block)
            pGroup->flowType = FlowType::Block;
        else
            pGroup->flowType = FlowType::Flow;
		pGroup->indent = GetIndent();

		m_groups.push(pGroup);
	}
	
	void EmitterState::EndedGroup(GroupType::value type)
	{
		if(m_groups.empty()) {
            if(type == GroupType::Seq)
                return SetError(ErrorMsg::UNEXPECTED_END_SEQ);
            else
                return SetError(ErrorMsg::UNEXPECTED_END_MAP);
        }
		
		// get rid of the current group
		{
			std::auto_ptr<Group> pFinishedGroup = m_groups.pop();
			if(pFinishedGroup->type != type)
				return SetError(ErrorMsg::UNMATCHED_GROUP_TAG);
		}

		// reset old settings
		unsigned lastIndent = (m_groups.empty() ? 0 : m_groups.top().indent);
		assert(m_curIndent >= lastIndent);
		m_curIndent -= lastIndent;
		
		// some global settings that we changed may have been overridden
		// by a local setting we just popped, so we need to restore them
		m_globalModifiedSettings.restore();

        ClearModifiedSettings();
	}

    EmitterNodeType::value EmitterState::CurGroupNodeType() const
    {
        if(m_groups.empty())
            return EmitterNodeType::None;
        
        return m_groups.top().NodeType();
    }

    GroupType::value EmitterState::CurGroupType() const
	{
        return m_groups.empty() ? GroupType::None : m_groups.top().type;
	}
	
    FlowType::value EmitterState::CurGroupFlowType() const
	{
        return m_groups.empty() ? FlowType::None : m_groups.top().flowType;
	}

    int EmitterState::CurGroupIndent() const
    {
        return m_groups.empty() ? 0 : m_groups.top().indent;
    }

    std::size_t EmitterState::CurGroupChildCount() const
    {
        return m_groups.empty() ? m_docCount : m_groups.top().childCount;
    }

    bool EmitterState::CurGroupLongKey() const
    {
        return m_groups.empty() ? false : m_groups.top().longKey;
    }

    int EmitterState::LastIndent() const
    {
        if(m_groups.size() <= 1)
            return 0;
        
        return m_curIndent - m_groups.top(-1).indent;
    }

	void EmitterState::ClearModifiedSettings()
	{
		m_modifiedSettings.clear();
	}

	bool EmitterState::SetOutputCharset(EMITTER_MANIP value, FmtScope::value scope)
	{
		switch(value) {
			case EmitNonAscii:
			case EscapeNonAscii:
				_Set(m_charset, value, scope);
				return true;
			default:
				return false;
		}
	}
	
	bool EmitterState::SetStringFormat(EMITTER_MANIP value, FmtScope::value scope)
	{
		switch(value) {
			case Auto:
			case SingleQuoted:
			case DoubleQuoted:
			case Literal:
				_Set(m_strFmt, value, scope);
				return true;
			default:
				return false;
		}
	}
	
	bool EmitterState::SetBoolFormat(EMITTER_MANIP value, FmtScope::value scope)
	{
		switch(value) {
			case OnOffBool:
			case TrueFalseBool:
			case YesNoBool:
				_Set(m_boolFmt, value, scope);
				return true;
			default:
				return false;
		}
	}

	bool EmitterState::SetBoolLengthFormat(EMITTER_MANIP value, FmtScope::value scope)
	{
		switch(value) {
			case LongBool:
			case ShortBool:
				_Set(m_boolLengthFmt, value, scope);
				return true;
			default:
				return false;
		}
	}

	bool EmitterState::SetBoolCaseFormat(EMITTER_MANIP value, FmtScope::value scope)
	{
		switch(value) {
			case UpperCase:
			case LowerCase:
			case CamelCase:
				_Set(m_boolCaseFmt, value, scope);
				return true;
			default:
				return false;
		}
	}

	bool EmitterState::SetIntFormat(EMITTER_MANIP value, FmtScope::value scope)
	{
		switch(value) {
			case Dec:
			case Hex:
			case Oct:
				_Set(m_intFmt, value, scope);
				return true;
			default:
				return false;
		}
	}

	bool EmitterState::SetIndent(unsigned value, FmtScope::value scope)
	{
		if(value <= 1)
			return false;
		
		_Set(m_indent, value, scope);
		return true;
	}

	bool EmitterState::SetPreCommentIndent(unsigned value, FmtScope::value scope)
	{
		if(value == 0)
			return false;
		
		_Set(m_preCommentIndent, value, scope);
		return true;
	}
	
	bool EmitterState::SetPostCommentIndent(unsigned value, FmtScope::value scope)
	{
		if(value == 0)
			return false;
		
		_Set(m_postCommentIndent, value, scope);
		return true;
	}

	bool EmitterState::SetFlowType(GroupType::value groupType, EMITTER_MANIP value, FmtScope::value scope)
	{
		switch(value) {
			case Block:
			case Flow:
				_Set(groupType == GroupType::Seq ? m_seqFmt : m_mapFmt, value, scope);
				return true;
			default:
				return false;
		}
	}

	EMITTER_MANIP EmitterState::GetFlowType(GroupType::value groupType) const
	{
		// force flow style if we're currently in a flow
		if(CurGroupFlowType() == FlowType::Flow)
			return Flow;
		
		// otherwise, go with what's asked of us
		return (groupType == GroupType::Seq ? m_seqFmt.get() : m_mapFmt.get());
	}
	
	bool EmitterState::SetMapKeyFormat(EMITTER_MANIP value, FmtScope::value scope)
	{
		switch(value) {
			case Auto:
			case LongKey:
				_Set(m_mapKeyFmt, value, scope);
				return true;
			default:
				return false;
		}
	}

    bool EmitterState::SetFloatPrecision(int value, FmtScope::value scope)
    {
        if(value < 0 || value > std::numeric_limits<float>::digits10 + 1)
            return false;
        _Set(m_floatPrecision, value, scope);
        return true;
    }
    
    bool EmitterState::SetDoublePrecision(int value, FmtScope::value scope)
    {
        if(value < 0 || value > std::numeric_limits<double>::digits10 + 1)
            return false;
        _Set(m_doublePrecision, value, scope);
        return true;
    }
}

