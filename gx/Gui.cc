//
// gx/Gui.cc
// Copyright (C) 2021 Richard Bradley
//

// TODO - handle tab/enter/mouse select differently for entry
// TODO - cursor movement for entry
// TODO - allow right button to open menus & select menu items
// TODO - 'stay open' logic for menus (quick click/release on menu?)

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
  [[nodiscard]] bool contains(const gx::GuiElem& e, float x, float y)
  {
    return (x >= e._x) && (x < (e._x + e._w))
      && (y >= e._y) && (y < (e._y + e._h));
  }

  [[nodiscard]] float actualX(float x, float w, gx::AlignEnum a)
  {
    if (gx::HAlign(a) == gx::ALIGN_LEFT) {
      return x;
    } else if (gx::HAlign(a) == gx::ALIGN_RIGHT) {
      return x - w;
    } else {
      return x - (w / 2.0f);
    }
  }

  [[nodiscard]] float actualY(float y, float h, gx::AlignEnum a)
  {
    if (gx::VAlign(a) == gx::ALIGN_TOP) {
      return y;
    } else if (gx::VAlign(a) == gx::ALIGN_BOTTOM) {
      return y - h;
    } else {
      return y - (h / 2.0f);
    }
  }

  [[nodiscard]] int calcLines(std::string_view text)
  {
    if (text.empty()) { return 0; }
    int lines = 1;
    for (int ch : text) { lines += (ch == '\n'); }
    return lines;
  }

  void deactivate(gx::GuiElem& def)
  {
    def._active = false;
    for (auto& e : def.elems) { deactivate(e); }
  }

  [[nodiscard]] gx::GuiElem* findElemByXY(
    gx::GuiElem& def, float x, float y, bool popup)
  {
    if (popup && def.type == gx::GUI_MENU) {
      if (def._active) {
        gx::GuiElem* e = findElemByXY(def.elems[1], x, y, false);
        if (e) { return e; }
      }
      if (contains(def, x, y)) { return &def; }
    }
    else if (popup || contains(def, x, y))
    {
      if (!popup && def.id != 0) { return &def; }
      for (gx::GuiElem& c : def.elems) {
        gx::GuiElem* e = findElemByXY(c, x, y, popup);
        if (e) { return e; }
      }
    }
    return nullptr;
  }
}


gx::Gui::Gui(const GuiElem& rootElem)
  : _rootElem(rootElem) { init(_rootElem); }

gx::Gui::Gui(GuiElem&& rootElem)
  : _rootElem(std::move(rootElem)) { init(_rootElem); }

void gx::Gui::layout(const GuiTheme& theme, float x, float y, AlignEnum align)
{
  _theme = theme;
  assert(_theme.baseFont != nullptr);

  _pt.set(x, y);
  _rootElem.align = align;
  _needLayout = true;
  _needRender = true;
}

void gx::Gui::update(Window& win)
{
  // clear event state that only persists for a single update
  _pressedID = 0;
  _releasedID = 0;
  _entryID = 0;
  _needRedraw = false;

  // size & position update
  if (_needLayout) {
    calcSize(_rootElem);
    float x = actualX(_pt.x, _rootElem._w, _rootElem.align);
    float y = actualY(_pt.y, _rootElem._h, _rootElem.align);
    calcPos(_rootElem, x, y);
    _needLayout = false;
  }

  // mouse movement/button handling
  if (win.focused()
      && (win.events() & (EVENT_MOUSE_MOVE | EVENT_MOUSE_BUTTON1))) {
    processMouseEvent(win);
  }

  // entry input handling & cursor update
  if (_focusID != 0) {
    if (win.events() & EVENT_CHAR) { processCharEvent(win); }

    if (_pressedID != 0) {
      // reset cursor blink state
      _lastCursorUpdate = win.lastPollTime();
      _cursorState = true;
    } else if (_theme.cursorBlinkTime > 0) {
      // check for cursor blink
      int64_t blinks = (win.lastPollTime() - _lastCursorUpdate)
        / int64_t(_theme.cursorBlinkTime);
      if (blinks > 0) {
        _lastCursorUpdate += blinks * _theme.cursorBlinkTime;
        if (blinks & 1) { _cursorState = !_cursorState; }
        _needRender = true;
      }
    }
  }

  // redraw GUI if needed
  if (_needRender) {
    // layers:
    // 0 - background/frames
    // 1 - text
    // 2 - popup background/frames
    // 3 - popup text

    DrawContext dc0(_dlm[0]), dc1(_dlm[1]);
    dc0.clear();
    dc1.clear();

    drawRec(dc0, _rootElem, _theme.colorBackground);
    drawElem(dc0, dc1, _rootElem, BSTATE_NONE, false);

    if (_popupActive) {
      DrawContext dc2(_dlm[2]), dc3(_dlm[3]);
      dc2.clear();
      dc3.clear();

      drawElem(dc2, dc3, _rootElem, BSTATE_NONE, true);
    } else {
      // clear popups
      auto itr = _dlm.find(2);
      if (itr != _dlm.end()) { itr->second.clear(); }
      itr = _dlm.find(3);
      if (itr != _dlm.end()) { itr->second.clear(); }
    }

    _needRender = false;
    _needRedraw = true;
  }
}

void gx::Gui::processMouseEvent(Window& win)
{
  const bool buttonDown = win.buttons() & BUTTON1;
  const bool buttonEvent = win.events() & EVENT_MOUSE_BUTTON1;

  // get elem at pointer
  GuiElem* ptr = nullptr;
  int id = 0;
  GuiElemType type = GUI_NULL;
  if (win.mouseIn()) {
    ptr = findElemByXY(_rootElem, win.mouseX(), win.mouseY(), _popupActive);
    if (ptr) {
      id = ptr->id;
      type = ptr->type;
    }
  }

  // update hoverID
  if (_hoverID != id) {
    int hid = id;
    if (buttonDown && _heldType != GUI_MENU && id != _heldID) { hid = 0; }
    if (_hoverID != hid) {
      _hoverID = hid;
      _needRender = true;
    }
  }

  if (buttonDown) {
    // clear focus if button clicked in another GUI
    if (win.removedEvents() & EVENT_MOUSE_BUTTON1) {
      setFocusID(0);
    }

    // update pressedID
    if (buttonEvent
        // treat moving between menus with button held as a press event
        || (type == GUI_MENU && _heldID != id && _heldType == GUI_MENU))
    {
      setFocusID((type == GUI_ENTRY) ?  id : 0);
      _pressedID = id;
      _heldID = id;
      _heldType = type;
      if (type == GUI_MENU) {
        if (_popupActive) { deactivate(_rootElem); } // close other open menu
        ptr->_active = true;
        _popupActive = true;
        _needRender = true;
      } else if (_pressedID != 0) {
        _needRender = true;
      }
    }
  } else {
    // update releasedID
    if (buttonEvent) {
      if ((_heldID == id) || (type == GUI_MENU_ITEM)) { _releasedID = id; }

      if (_heldID != 0) {
        _heldID = 0;
        _heldType = GUI_NULL;
        _needRender = true;
      }

      if (_popupActive) {
        // close open menus
        deactivate(_rootElem);
        _popupActive = false;
        _needRender = true;
      }
    }
  }

  // clear button event if used by GUI
  if (buttonEvent && (_pressedID != 0 || _releasedID != 0)) {
    win.removeEvent(EVENT_MOUSE_BUTTON1);
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
        if (code > 31 && (e->entry.maxLength == 0 || len < e->entry.maxLength)) {
          e->text += toUTF8(code);
          ++len;
          _needRender = true;
        }
      }
    } else if ((c.key == KEY_TAB && c.mods == 0) || c.key == KEY_ENTER) {
      GuiElem* next = findNextElem(_focusID, GUI_ENTRY);
      setFocusID(next ? next->id : 0);
      usedEvent = true;
    } else if (c.key == KEY_TAB && c.mods == MOD_SHIFT) {
      GuiElem* prev = findPrevElem(_focusID, GUI_ENTRY);
      setFocusID(prev ? prev->id : 0);
      usedEvent = true;
    }
  }

  if (usedEvent) {
    win.removeEvent(EVENT_CHAR);
    // reset cursor blink state
    _lastCursorUpdate = win.lastPollTime();
    _cursorState = true;
  }
}

void gx::Gui::setFocusID(int id)
{
  if (_focusID == id) { return; }
  if ( _textChanged) {
    _textChanged = false;
    _entryID = _focusID;
  }
  _focusID = id;
  _needRender = true;
}

bool gx::Gui::setText(int id, std::string_view text)
{
  GuiElem* e = findElem(id);
  if (!e) { return false; }

  e->text = text;
  _needLayout = true;
  _needRender = true;
  return true;
}

void gx::Gui::init(GuiElem& def)
{
  if (def.type == GUI_MENU) {
    def.id = _uniqueID--;
  }

#ifndef NDEBUG
  switch (def.type) {
    case GUI_BUTTON:
    case GUI_MENU_ITEM:
      assert(def.elems.size() == 1);
      break;
    case GUI_MENU:
      assert(def.elems.size() == 2);
      break;
    default:
      break;
  }
#endif

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
      const Font& fnt = *_theme.baseFont;
      def._w = fnt.calcWidth(def.text);
      int lines = calcLines(def.text);
      def._h = float(
        (fnt.size() - 1) * lines + (_theme.spacing * std::max(lines - 1, 0)));
      // FIXME - improve line height calc (based on font ymax/ymin?)
      break;
    }
    case GUI_HLINE:
      def._w = float(32 + _theme.border*2);
      def._h = float(1 + _theme.border*2);
      break;
    case GUI_VLINE:
      def._w = float(1 + _theme.border*2);
      def._h = float(32 + _theme.border*2);
      break;
    case GUI_BUTTON:
    case GUI_MENU_ITEM: {
      GuiElem& e = def.elems[0];
      calcSize(e);
      def._w = float(e._w + (_theme.border * 2));
      def._h = float(e._h + (_theme.border * 2));
      break;
    }
    case GUI_MENU: {
      // menu button
      GuiElem& e = def.elems[0];
      calcSize(e);
      def._w = float(e._w + (_theme.border * 2));
      def._h = float(e._h + (_theme.border * 2));
      // menu items
      calcSize(def.elems[1]);
      break;
    }
    case GUI_ENTRY: {
      const Font& fnt = *_theme.baseFont;
      def._w = def.entry.size * fnt.calcWidth("A");
      def._h = float(fnt.size());
      // FIXME - use better width value than capital A * size
      break;
    }
    case GUI_IMAGE:
      def._w = def.image.width + (_theme.border * 2);
      def._h = def.image.height + (_theme.border * 2);
      break;
    default:
      GX_LOG_ERROR("unknown type ", def.type);
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
      GX_LOG_ERROR("unknown type ", def.type);
      break;
  }
}

void gx::Gui::drawElem(DrawContext& dc, DrawContext& dc2, const GuiElem& def,
                       ButtonState bstate, bool popup) const
{
  if (!popup) {
    switch (def.type) {
      case GUI_HFRAME:
      case GUI_VFRAME:
        break;
      case GUI_LABEL:
        // TODO - use bstate to select different theme values
        //    (i.e. hovering over a button causes label color change)
        dc2.color(_theme.colorText);
        dc2.text(*_theme.baseFont, def._x, def._y, ALIGN_TOP_LEFT,
                 _theme.spacing, def.text);
        break;
      case GUI_HLINE:
      case GUI_VLINE:
        dc.color(_theme.colorText);
        dc.rectangle(def._x + _theme.border, def._y + _theme.border,
                     def._w - (_theme.border*2), def._h - (_theme.border*2));
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
        } else if (def.id == _hoverID) {
          drawRec(dc, def, _theme.colorButtonHover);
          bstate = BSTATE_HOVER;
        } else {
          drawRec(dc, def, _theme.colorButtonNormal);
          bstate = BSTATE_NORMAL;
        }
        break;
      case GUI_MENU:
        if (def.id == _heldID) {
          drawRec(dc, def, _theme.colorMenuSelect);
          bstate = BSTATE_HELD_ONLY;
        } else if (def.id == _hoverID) {
          drawRec(dc, def, _theme.colorMenuHover);
          bstate = BSTATE_HOVER;
        } else {
          drawRec(dc, def, _theme.colorMenuNormal);
          bstate = BSTATE_NORMAL;
        }
        break;
      case GUI_MENU_HFRAME:
      case GUI_MENU_VFRAME:
        drawRec(dc, def, _theme.colorMenuItem);
        break;
      case GUI_MENU_ITEM:
        if (def.id == _hoverID) {
          drawRec(dc, def, _theme.colorMenuItemSelect);
        }
        break;
      case GUI_ENTRY: {
        const Font& fnt = *_theme.baseFont;
        float tw = fnt.calcWidth(def.text);
        if (def.id == _focusID) {
          drawRec(dc, def, _theme.colorEntryFocus);
          tw += float(1 + _theme.cursorWidth);
        } else {
          drawRec(dc, def, _theme.colorEntry);
        }
        float tx = def._x;
        if (tw > def._w) {
          // text doesn't fit in entry
          tx -= tw - def._w;
          dc2.hgradiant(def._x + 1.0f, _theme.colorText &  0x00ffffff,
                        def._x + float(fnt.size() / 2), _theme.colorText);
          dc2.text(fnt, tx, def._y, ALIGN_TOP_LEFT, _theme.spacing,
                   def.text, {def._x, def._y, def._w, def._h});
        } else {
          dc2.color(_theme.colorText);
          dc2.text(fnt, tx, def._y, ALIGN_TOP_LEFT, _theme.spacing, def.text);
        }
        dc2.text(fnt, tx, def._y, ALIGN_TOP_LEFT, _theme.spacing,
                 def.text, {def._x, def._y, def._w, def._h});
        if (def.id == _focusID && _cursorState) {
          // draw cursor
          dc.color(_theme.colorCursor);
          dc.rectangle(tx + tw - float(_theme.cursorWidth), def._y+1,
                       float(_theme.cursorWidth), float(fnt.size()-2));
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
        GX_LOG_ERROR("unknown type ", def.type);
        break;
    }
  }

  // draw child elements
  if (def.type != GUI_MENU) {
    for (auto& e : def.elems) { drawElem(dc, dc2, e, bstate, popup); }
  } else if (!popup) {
    drawElem(dc, dc2, def.elems[0], bstate, popup);
  } else if (def._active) {
    drawElem(dc, dc2, def.elems[1], bstate, false);
  }
}

void gx::Gui::drawRec(DrawContext& dc, const GuiElem& def, uint32_t col) const
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
