//
// gx/Gui.cc
// Copyright (C) 2021 Richard Bradley
//

#include "Gui.hh"
#include "Window.hh"
#include "DrawContext.hh"
#include "Font.hh"
#include "Unicode.hh"
#include "System.hh"
#include "Logger.hh"
#include "Print.hh"
#include <algorithm>
#include <cassert>

namespace {
  constexpr bool contains(const gx::GuiElem& e, float x, float y)
  {
    return (x >= e._x) && (x < (e._x + e._w))
      && (y >= e._y) && (y < (e._y + e._h));
  }

  constexpr float actualX(float x, float w, gx::AlignEnum a)
  {
    if (gx::HAlign(a) == gx::ALIGN_LEFT) {
      return x;
    } else if (gx::HAlign(a) == gx::ALIGN_RIGHT) {
      return x - w;
    } else {
      return x - (w / 2.0f);
    }
  }

  constexpr float actualY(float y, float h, gx::AlignEnum a)
  {
    if (gx::VAlign(a) == gx::ALIGN_TOP) {
      return y;
    } else if (gx::VAlign(a) == gx::ALIGN_BOTTOM) {
      return y - h;
    } else {
      return y - (h / 2.0f);
    }
  }
}


gx::Gui::Gui(const GuiElem& rootElem) : _rootElem(rootElem) { init(_rootElem); }
gx::Gui::Gui(GuiElem&& rootElem) : _rootElem(std::move(rootElem)) { init(_rootElem); }

void gx::Gui::layout(const GuiTheme& theme, float x, float y, AlignEnum align)
{
  _theme = theme;
  _pt.set(x, y);
  _rootElem.align = align;
  _needSize = true;
  _needPos = true;
  _needRender = true;
  _needRedraw = true;
}

void gx::Gui::update(Window& win)
{
  // clear event state that only persists for a single update
  _releasedID = 0;
  _entryID = 0;

  // size & position update
  if (_needSize) {
    calcSize(_rootElem);
    _needSize = false;
  }

  if (_needPos) {
    float x = actualX(_pt.x, _rootElem._w, _rootElem.align);
    float y = actualY(_pt.y, _rootElem._h, _rootElem.align);
    calcPos(_rootElem, x, y);
    _needPos = false;
  }

  // button event & state update
  const bool buttonDown = win.buttons() & BUTTON1;
  const bool buttonEvent = win.events() & EVENT_MOUSE_BUTTON1;
  GuiElem* ptr = nullptr;
  int id = 0;
  GuiElemType type = GUI_NULL;
  if (win.mouseIn() && win.focused()) {
    ptr = findElem(win.mouseX(), win.mouseY());
    if (ptr) {
      id = ptr->id;
      type = ptr->type;
    }
  }

  // update hoverID
  if (_hoverID != id && (!buttonDown || _lastPressedID != 0)) {
    _hoverID = id;
    _needRender = true;
  }

  // update pressedID
  if (buttonDown && (win.removedEvents() & EVENT_MOUSE_BUTTON1) && _focusID) {
    // clear focus if button clicked in another GUI
    setFocusID(0);
    _needRender = true;
  }

  if (buttonDown
      && (buttonEvent
          // treat moving between menus with button held as a press event
          || (type == GUI_MENU && _lastPressedID != id && _lastType == GUI_MENU)))
  {
    setFocusID((type == GUI_ENTRY) ?  id : 0);
    _lastCursorUpdate = win.lastPollTime();
    _cursorState = true;
    _pressedID = id;
    _lastPressedID = id;
    _lastType = type;
    if (type == GUI_MENU) {
      deactivate(_rootElem); // close open menus
      ptr->_active = true;
    }
    _needRender = true;
  } else {
    _pressedID = 0;
  }

  // update releasedID
  if (!buttonDown && buttonEvent) {
    if ((_lastPressedID == id) || (type == GUI_MENU_ITEM)) { _releasedID = id; }
    _lastPressedID = 0;
    _lastType = GUI_NULL;
    deactivate(_rootElem); // close open menus
    _needRender = true;
  }

  // update heldID
  _heldID = buttonDown ? _lastPressedID : 0;

  // clear button event if used by GUI
  if (buttonEvent && (_pressedID != 0 || _releasedID != 0)) {
    win.removeEvent(EVENT_MOUSE_BUTTON1);
  }

  // char input
  if (_focusID && (win.events() & EVENT_CHAR)) {
    processCharEvent(win);
  }

  // cursor blink update
  if (_focusID && _theme.cursorBlinkTime > 0) {
    int64_t blinks =
      (win.lastPollTime() - _lastCursorUpdate) / int64_t(_theme.cursorBlinkTime);
    if (blinks > 0) {
      _lastCursorUpdate += blinks * _theme.cursorBlinkTime;
      if (blinks & 1) { _cursorState = !_cursorState; }
      _needRender = true;
    }
  }

  // redraw GUI if needed
  if (_needRender) {
    _dl.clear();
    DrawContext dc(_dl);
    drawRec(dc, _rootElem, _theme.colorBackground);
    drawElem(dc, _rootElem, BSTATE_NONE);
    _needRender = false;
    _needRedraw = true;
  } else {
    _needRedraw = false;
  }
}

void gx::Gui::processCharEvent(Window& win)
{
  GuiElem* e = findElem(_focusID);
  if (!e) { return; }
  assert(e->type == GUI_ENTRY);

  bool usedEvent = false;
  int len = lengthUTF8(e->text);
  for (const CharInfo& c : win.charData()) {
    if (c.codepoint) {
      usedEvent = true;
      if (e->entry.maxLength == 0 || len < e->entry.maxLength) {
        e->text += toUTF8(c.codepoint);
        ++len;
        _needRender = true;
        _textChanged = true;
        //println("char: ", c.codepoint);
      }
    } else if (c.key == KEY_BACKSPACE) {
      usedEvent = true;
      if (!e->text.empty()) {
        if (c.mods == MOD_ALT) {
          e->text.clear();
          len = 0;
        } else {
          popbackUTF8(e->text);
          --len;
        }
        _needRender = true;
        _textChanged = true;
        //println("backspace");
      }
    } else if (c.key == KEY_V && c.mods == MOD_CONTROL) {
      // (CTRL-V) paste first line of clipboard
      usedEvent = true;
      std::string cb = getClipboard();
      std::string_view line(cb.data(), cb.find('\n'));
      for (UTF8Iterator itr(line); !itr.done(); itr.next()) {
        int code = itr.get();
        if (code >= 32 && (e->entry.maxLength == 0 || len < e->entry.maxLength)) {
          e->text += toUTF8(code);
          ++len;
          _needRender = true;
        }
      }
    } else if ((c.key == KEY_TAB && c.mods == 0) || c.key == KEY_ENTER) {
      GuiElem* next = findNextElem(_focusID, GUI_ENTRY);
      setFocusID(next ? next->id : 0);
      _needRender = true;
      usedEvent = true;
    } else if (c.key == KEY_TAB && c.mods == MOD_SHIFT) {
      GuiElem* prev = findPrevElem(_focusID, GUI_ENTRY);
      setFocusID(prev ? prev->id : 0);
      _needRender = true;
      usedEvent = true;
    }
  }

  if (usedEvent) {
    _lastCursorUpdate = win.lastPollTime();
    _cursorState = true;
    win.removeEvent(EVENT_CHAR);
  }
}

void gx::Gui::setFocusID(int id)
{
  if (_focusID != id && _textChanged) {
    _textChanged = false;
    _entryID = _focusID;
  }
  _focusID = id;
}

bool gx::Gui::setText(int id, std::string_view text)
{
  GuiElem* e = findElem(id);
  if (!e) { return false; }

  e->text = text;
  _needRender = true;
  _needSize = true;
  _needPos = true;
  return true;
}

void gx::Gui::init(GuiElem& def)
{
  if (def.type == GUI_MENU) {
    def.id = _uniqueID--;
  }
  for (GuiElem& e : def.elems) { init(e); }
}

void gx::Gui::calcSize(GuiElem& def)
{
  switch (def.type) {
    case GUI_HFRAME:
    case GUI_MENU_HFRAME: {
      float total_w = 0, max_h = 0;
      for (GuiElem& e : def.elems) {
        calcSize(e);
        total_w += e._w + _theme.border;
        max_h = std::max(max_h, e._h);
      }
      if (def.type == GUI_MENU_HFRAME) {
        for (GuiElem& e : def.elems) { e._h = max_h; }
      }
      def._w = total_w + _theme.border;
      def._h = max_h + (_theme.border * 2);
      break;
    }
    case GUI_VFRAME:
    case GUI_MENU_VFRAME: {
      float total_h = 0, max_w = 0;
      for (GuiElem& e : def.elems) {
        calcSize(e);
        total_h += e._h + _theme.border;
        max_w = std::max(max_w, e._w);
      }
      if (def.type == GUI_MENU_VFRAME) {
        for (GuiElem& e : def.elems) { e._w = max_w; }
      }
      def._w = max_w + (_theme.border * 2);
      def._h = total_h + _theme.border;
      break;
    }
    case GUI_LABEL: {
      assert(_theme.baseFont != nullptr);
      const Font& fnt = *_theme.baseFont;
      def._w = fnt.calcWidth(def.text);
      int lines = fnt.calcLines(def.text);
      def._h = (fnt.size() - 1) * lines
        + (_theme.spacing * std::max(lines - 1, 0));
      // FIXME - improve line height calc (based on font ymax/ymin?)
      break;
    }
    case GUI_HLINE:
      def._w = 32 + _theme.border*2;
      def._h = 1 + _theme.border*2;
      break;
    case GUI_VLINE:
      def._w = 1 + _theme.border*2;
      def._h = 32 + _theme.border*2;
      break;
    case GUI_BUTTON:
    case GUI_MENU_ITEM: {
      assert(def.elems.size() == 1);
      GuiElem& e = def.elems[0];
      calcSize(e);
      def._w = e._w + (_theme.border * 2);
      def._h = e._h + (_theme.border * 2);
      break;
    }
    case GUI_MENU: {
      assert(def.elems.size() == 2);
      // menu button
      GuiElem& e = def.elems[0];
      calcSize(e);
      def._w = e._w + (_theme.border * 2);
      def._h = e._h + (_theme.border * 2);
      // menu items
      calcSize(def.elems[1]);
      break;
    }
    case GUI_ENTRY: {
      assert(_theme.baseFont != nullptr);
      const Font& fnt = *_theme.baseFont;
      def._w = def.entry.size * fnt.calcWidth("A");
      def._h = fnt.size();
      // FIXME - use better width value than capital A * size
      break;
    }
    case GUI_IMAGE:
      def._w = def.image.width + (_theme.border * 2);
      def._h = def.image.height + (_theme.border * 2);
      break;
    default:
      LOG_ERROR("unknown type ", def.type);
      break;
  }
}

void gx::Gui::calcPos(GuiElem& def, float base_x, float base_y)
{
  def._x = base_x;
  def._y = base_y;

  switch (def.type) {
    case GUI_HFRAME:
    case GUI_MENU_HFRAME:
      for (GuiElem& e : def.elems) {
        base_x += _theme.border;
        float yy = 0;
        if (VAlign(e.align) == ALIGN_TOP) {
          yy = _theme.border;
        } else if (VAlign(e.align) == ALIGN_BOTTOM) {
          yy = (def._h - e._h) - _theme.border;
        } else {
          yy = (def._h - e._h) / 2.0f;
        }
        calcPos(e, base_x, base_y + yy);
        base_x += e._w;
      }
      break;
    case GUI_VFRAME:
    case GUI_MENU_VFRAME:
      for (GuiElem& e : def.elems) {
        base_y += _theme.border;
        float xx = 0;
        if (HAlign(e.align) == ALIGN_LEFT) {
          xx = _theme.border;
        } else if (HAlign(e.align) == ALIGN_RIGHT) {
          xx = (def._w - e._w) - _theme.border;
        } else {
          xx = (def._w - e._w) / 2.0f;
        }
        calcPos(e, base_x + xx, base_y);
        base_y += e._h;
      }
      break;
    case GUI_BUTTON:
    case GUI_MENU_ITEM:
      calcPos(def.elems[0], base_x + _theme.border, base_y + _theme.border);
      break;
    case GUI_MENU:
      calcPos(def.elems[0], base_x + _theme.border, base_y + _theme.border);
      // FIXME - menu items always directly under button for now
      calcPos(def.elems[1], base_x, base_y + def._h);
      break;
    case GUI_LABEL:
    case GUI_HLINE:
    case GUI_VLINE:
    case GUI_ENTRY:
    case GUI_IMAGE:
      // noting extra to do
      break;
    default:
      LOG_ERROR("unknown type ", def.type);
      break;
  }
}

void gx::Gui::deactivate(GuiElem& def)
{
  def._active = false;
  for (GuiElem& e : def.elems) { deactivate(e); }
}

void gx::Gui::drawElem(DrawContext& dc, GuiElem& def, ButtonState bstate)
{
  switch (def.type) {
    case GUI_HFRAME:
    case GUI_VFRAME:
      for (GuiElem& e : def.elems) { drawElem(dc, e, bstate); }
      break;
    case GUI_MENU_HFRAME:
    case GUI_MENU_VFRAME:
      drawRec(dc, def, _theme.colorMenuItem);
      for (GuiElem& e : def.elems) { drawElem(dc, e, bstate); }
      break;
    case GUI_LABEL:
      dc.color(_theme.colorText);
      dc.text(*_theme.baseFont, def._x, def._y, ALIGN_TOP_LEFT,
               _theme.spacing, def.text);
      break;
    case GUI_HLINE:
    case GUI_VLINE:
      dc.color(_theme.colorText);
      dc.rectangle(def._x + _theme.border, def._y + _theme.border, def._w - (_theme.border*2), def._h - (_theme.border*2));
      break;
    case GUI_BUTTON:
      if (def.id == _heldID) {
        if (def.id == _hoverID) {
          drawRec(dc, def, _theme.colorButtonPressed);
          bstate = BSTATE_PRESSED;
        } else {
          drawRec(dc, def, _theme.colorButtonHeldOnly);
          bstate = BSTATE_HELD_ONLY;
        }
      } else if (def.id == _hoverID && !_heldID) {
        drawRec(dc, def, _theme.colorButtonHover);
        bstate = BSTATE_HOVER;
      } else {
        drawRec(dc, def, _theme.colorButtonNormal);
        bstate = BSTATE_NORMAL;
      }
      drawElem(dc, def.elems[0], bstate);
      break;
    case GUI_MENU:
      if (def.id == _heldID) {
        drawRec(dc, def, _theme.colorMenuSelect);
        bstate = BSTATE_HELD_ONLY;
      } else if (def.id == _hoverID && !_heldID) {
        drawRec(dc, def, _theme.colorMenuHover);
        bstate = BSTATE_HOVER;
      } else {
        drawRec(dc, def, _theme.colorMenuNormal);
        bstate = BSTATE_NORMAL;
      }
      drawElem(dc, def.elems[0], bstate);
      if (def._active) { drawElem(dc, def.elems[1], bstate); }
      break;
    case GUI_MENU_ITEM:
      if (def.id == _hoverID) {
        drawRec(dc, def, _theme.colorMenuItemSelect);
      }
      drawElem(dc, def.elems[0], bstate);
      break;
    case GUI_ENTRY: {
      const Font& fnt = *_theme.baseFont;
      float tw = fnt.calcWidth(def.text);
      if (def.id == _focusID) {
        drawRec(dc, def, _theme.colorEntryFocus);
        tw += 1 + _theme.cursorWidth;
      } else {
        drawRec(dc, def, _theme.colorEntry);
      }
      float tx = def._x;
      if (tw > def._w) {
        // text doesn't fit in entry
        tx -= tw - def._w;
        dc.hgradiant(def._x + 1.0f, _theme.colorText &  0x00ffffff,
                     def._x + float(fnt.size() / 2), _theme.colorText);
        dc.text(fnt, tx, def._y, ALIGN_TOP_LEFT,
                _theme.spacing, def.text, {def._x, def._y, def._w, def._h});
      } else {
        dc.color(_theme.colorText);
        dc.text(fnt, tx, def._y, ALIGN_TOP_LEFT, _theme.spacing, def.text);
      }
      dc.text(fnt, tx, def._y, ALIGN_TOP_LEFT,
              _theme.spacing, def.text, {def._x, def._y, def._w, def._h});
      if (def.id == _focusID && _cursorState) {
        // draw cursor
        dc.color(_theme.colorCursor);
        dc.rectangle(tx + tw - _theme.cursorWidth, def._y+1,
                     _theme.cursorWidth, fnt.size()-2);
      }
      break;
    }
    case GUI_IMAGE:
      dc.texture(def.image.texId);
      dc.rectangle(def._x + _theme.border, def._y + _theme.border,
                   def.image.width, def.image.height,
                   def.image.texCoord0, def.image.texCoord1);
      break;
    default:
      LOG_ERROR("unknown type ", def.type);
      break;
  }
}

gx::GuiElem* gx::Gui::findElem(float x, float y)
{
  // - full search through all elems needed because of popup elements
  // - only GuiElem with an id can be found
  std::vector<GuiElem*> stack;
  GuiElem* e = &_rootElem;
  while (e->id == 0 || !contains(*e, x, y)) {
    if (e->type != GUI_MENU || e->_active) {
      stack.reserve(stack.size() + e->elems.size());
      for (GuiElem& c : e->elems) { stack.push_back(&c); }
    }

    if (stack.empty()) { return nullptr; }
    e = stack.back();
    stack.pop_back();
  }
  return e;
}

namespace {
  template<class T>
  inline T* findElemT(T* root, int id)
  {
    assert(id != 0);
    std::vector<T*> stack;
    T* e = root;
    while (e->id != id) {
      if (!e->elems.empty()) {
        stack.reserve(stack.size() + e->elems.size());
        for (T& c : e->elems) { stack.push_back(&c); }
      }

      if (stack.empty()) { return nullptr; }
      e = stack.back();
      stack.pop_back();
    }
    return e;
  }
}

gx::GuiElem* gx::Gui::findElem(int id)
{
  return findElemT(&_rootElem, id);
}

const gx::GuiElem* gx::Gui::findElem(int id) const
{
  return findElemT(&_rootElem, id);
}

gx::GuiElem* gx::Gui::findNextElem(int id, GuiElemType type)
{
  assert(id != 0);
  std::vector<GuiElem*> stack;
  GuiElem* e = &_rootElem;
  GuiElem* next = nullptr;
  GuiElem* first = nullptr;
  for (;;) {
    if (e->id > 0 && (type == GUI_NULL || e->type == type)) {
      if (e->id == (id+1)) { return e; }
      if (e->id > id && (!next || e->id < next->id)) { next = e; }
      if (!first || e->id < first->id) { first = e; }
    }

    if (!e->elems.empty()) {
      stack.reserve(stack.size() + e->elems.size());
      for (GuiElem& c : e->elems) { stack.push_back(&c); }
    }

    if (stack.empty()) { return next ? next : first; }
    e = stack.back();
    stack.pop_back();
  }
}

gx::GuiElem* gx::Gui::findPrevElem(int id, GuiElemType type)
{
  assert(id != 0);
  std::vector<GuiElem*> stack;
  GuiElem* e = &_rootElem;
  GuiElem* prev = nullptr;
  GuiElem* last = nullptr;
  for (;;) {
    if (e->id > 0 && (type == GUI_NULL || e->type == type)) {
      if (e->id == (id-1)) { return e; }
      if (e->id < id && (!prev || e->id > prev->id)) { prev = e; }
      if (!last || e->id > last->id) { last = e; }
    }

    if (!e->elems.empty()) {
      stack.reserve(stack.size() + e->elems.size());
      for (GuiElem& c : e->elems) { stack.push_back(&c); }
    }

    if (stack.empty()) { return prev ? prev : last; }
    e = stack.back();
    stack.pop_back();
  }
}

void gx::Gui::drawRec(DrawContext& dc, const GuiElem& def, uint32_t col)
{
  if (col != 0) {
    dc.color(col);
    dc.rectangle(def._x, def._y, def._w, def._h);
  }

  if (_theme.colorEdge != 0) {
    dc.color(_theme.colorEdge);
    dc.rectangle(def._x, def._y, def._w, 1);
    dc.rectangle(def._x, def._y + def._h - 1, def._w, 1);
    dc.rectangle(def._x, def._y, 1, def._h);
    dc.rectangle(def._x + def._w - 1, def._y, 1, def._h);
  }
}
