// Scintilla source code edit control
/** @file CellBuffer.h
 ** Manages the text of the document.
 **/
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CELLBUFFER_H
#define CELLBUFFER_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

// Interface to per-line data that wants to see each line insertion and deletion
class PerLine {
public:
	virtual ~PerLine() {}
	virtual void Init()=0;
	virtual void InsertLine(int line)=0;
	virtual void RemoveLine(int line)=0;
};

/**
 * The line vector contains information about each of the lines in a cell buffer.
 */
class LineVector {

	Partitioning starts;
	PerLine *perLine;

public:

	LineVector();
	~LineVector();
	void Init();
	void SetPerLine(PerLine *pl);

	void InsertText(int line, int delta);
	void InsertLine(int line, int position, bool lineStart);
	void SetLineStart(int line, int position);
	void RemoveLine(int line);
	int Lines() const {
		return starts.Partitions();
	}
	int LineFromPosition(int pos) const;
	int LineStart(int line) const {
		return starts.PositionFromPartition(line);
	}

	int MarkValue(int line);
	int AddMark(int line, int marker);
	void MergeMarkers(int pos);
	void DeleteMark(int line, int markerNum, bool all);
	void DeleteMarkFromHandle(int markerHandle);
	int LineFromHandle(int markerHandle);

	void ClearLevels();
	int SetLevel(int line, int level);
	int GetLevel(int line);

	int SetLineState(int line, int state);
	int GetLineState(int line);
	int GetMaxLineState();

};

enum actionType { insertAction, removeAction, startAction, containerAction };

/**
 * Actions are used to store all the information required to perform one undo/redo step.
 */
class Action {
public:
	actionType at;
	int position;
	char *data;
	int lenData;
	bool mayCoalesce;

	Action();
	~Action();
	void Create(actionType at_, int position_=0, const char *data_=0, int lenData_=0, bool mayCoalesce_=true);
	void Destroy();
	void Grab(Action *source);
};

/**
 *
 */
class UndoHistory {
	Action *actions;
	int lenActions;
	int maxAction;
	int currentAction;
	int undoSequenceDepth;
	int savePoint;
	int tentativePoint;

	void EnsureUndoRoom();

	// Private so UndoHistory objects can not be copied
	UndoHistory(const UndoHistory &);

public:
	UndoHistory();
	~UndoHistory();

	const char *AppendAction(actionType at, int position, const char *data, int length, bool &startSequence, bool mayCoalesce=true);

	void BeginUndoAction();
	void EndUndoAction();
	void DropUndoSequence();
	void DeleteUndoHistory();

	/// The save point is a marker in the undo stack where the container has stated that
	/// the buffer was saved. Undo and redo can move over the save point.
	void SetSavePoint();
	bool IsSavePoint() const;

	// Tentative actions are used for input composition so that it can be undone cleanly
	void TentativeStart();
	void TentativeCommit();
	bool TentativeActive() const { return tentativePoint >= 0; }
	int TentativeSteps();

	/// To perform an undo, StartUndo is called to retrieve the number of steps, then UndoStep is
	/// called that many times. Similarly for redo.
	bool CanUndo() const;
	int StartUndo();
	const Action &GetUndoStep() const;
	void CompletedUndoStep();
	bool CanRedo() const;
	int StartRedo();
	const Action &GetRedoStep() const;
	void CompletedRedoStep();
};

/**
 * Holder for an expandable array of characters that supports undo and line markers.
 * Based on article "Data Structures in a Bit-Mapped Text Editor"
 * by Wilfred J. Hansen, Byte January 1987, page 183.
 */
class CellBuffer {
private:
	SplitVector<char> substance;
	SplitVector<char> style;
	bool readOnly;
	int utf8LineEnds;

	bool collectingUndo;
	UndoHistory uh;

	LineVector lv;

	bool UTF8LineEndOverlaps(int position) const;
	void ResetLineEnds();
	/// Actions without undo
	void BasicInsertString(int position, const char *s, int insertLength);
	void BasicDeleteChars(int position, int deleteLength);

public:

	CellBuffer();
	~CellBuffer();

	/// Retrieving positions outside the range of the buffer works and returns 0
	char CharAt(int position) const;
	void GetCharRange(char *buffer, int position, int lengthRetrieve) const;
	char StyleAt(int position) const;
	void GetStyleRange(unsigned char *buffer, int position, int lengthRetrieve) const;
	const char *BufferPointer();
	const char *RangePointer(int position, int rangeLength);
	int GapPosition() const;

	int Length() const;
	void Allocate(int newSize);
	int GetLineEndTypes() const { return utf8LineEnds; }
	void SetLineEndTypes(int utf8LineEnds_);
	void SetPerLine(PerLine *pl);
	int Lines() const;
	int LineStart(int line) const;
	int LineFromPosition(int pos) const { return lv.LineFromPosition(pos); }
	void InsertLine(int line, int position, bool lineStart);
	void RemoveLine(int line);
	const char *InsertString(int position, const char *s, int insertLength, bool &startSequence);

	/// Setting styles for positions outside the range of the buffer is safe and has no effect.
	/// @return true if the style of a character is changed.
	bool SetStyleAt(int position, char styleValue);
	bool SetStyleFor(int position, int length, char styleValue);

	const char *DeleteChars(int position, int deleteLength, bool &startSequence);

	bool IsReadOnly() const;
	void SetReadOnly(bool set);

	/// The save point is a marker in the undo stack where the container has stated that
	/// the buffer was saved. Undo and redo can move over the save point.
	void SetSavePoint();
	bool IsSavePoint() const;

	void TentativeStart();
	void TentativeCommit();
	bool TentativeActive() const;
	int TentativeSteps();

	bool SetUndoCollection(bool collectUndo);
	bool IsCollectingUndo() const;
	void BeginUndoAction();
	void EndUndoAction();
	void AddUndoAction(int token, bool mayCoalesce);
	void DeleteUndoHistory();

	/// To perform an undo, StartUndo is called to retrieve the number of steps, then UndoStep is
	/// called that many times. Similarly for redo.
	bool CanUndo() const;
	int StartUndo();
	const Action &GetUndoStep() const;
	void PerformUndoStep();
	bool CanRedo() const;
	int StartRedo();
	const Action &GetRedoStep() const;
	void PerformRedoStep();
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
