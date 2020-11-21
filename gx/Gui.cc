//
// gx/Gui.cc
// Copyright (C) 2020 Richard Bradley
//

#include "Gui.hh"
#include "Window.hh"
#include "Font.hh"
#include "Logger.hh"
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
    ptr = findEventElem(win.mouseX(), win.mouseY());
    if (ptr) {
      id = ptr->eventID;
      type = ptr->type;
    }
  }

  // update hoverID
  if (_hoverID != id && (!buttonDown || _lastPressedID != 0)) {
    _hoverID = id;
    _needRender = true;
  }

  // update pressedID
  if (buttonDown
      && (buttonEvent
          // treat moving between menus with button held as a press event
          || (type == GUI_MENU && _lastPressedID != id && _lastType == GUI_MENU)))
  {
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
  } else {
    _releasedID = 0;
  }

  // update heldID
  _heldID = buttonDown ? _lastPressedID : 0;

  // clear button event if used by GUI
  if (buttonEvent && (_pressedID != 0 || _releasedID != 0)) {
    win.removeEvent(gx::EVENT_MOUSE_BUTTON1);
  }

  // redraw GUI if needed
  if (_needRender) {
    _dl.clear();
    drawRec(_rootElem, _theme.colorBackground);
    drawElem(_rootElem, BSTATE_NONE);
    _needRender = false;
    _needRedraw = true;
  } else {
    _needRedraw = false;
  }
}

void gx::Gui::init(GuiElem& def)
{
  if (def.type == GUI_MENU) {
    def.eventID = _uniqueID--;
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

void gx::Gui::drawElem(GuiElem& def, ButtonState bstate)
{
  // FIXME - add style option to HFRAME,VFRAME,BUTTON

  switch (def.type) {
    case GUI_HFRAME:
    case GUI_VFRAME:
      for (GuiElem& e : def.elems) { drawElem(e, bstate); }
      break;
    case GUI_MENU_HFRAME:
    case GUI_MENU_VFRAME:
      drawRec(def, _theme.colorMenuItem);
      for (GuiElem& e : def.elems) { drawElem(e, bstate); }
      break;
    case GUI_LABEL:
      _dl.color(_theme.colorText);
      _dl.text(*_theme.baseFont, def._x, def._y, gx::ALIGN_TOP_LEFT,
               _theme.spacing, def.text);
      break;
    case GUI_HLINE:
    case GUI_VLINE:
      _dl.color(_theme.colorText);
      _dl.rectangle(def._x + _theme.border, def._y + _theme.border, def._w - (_theme.border*2), def._h - (_theme.border*2));
      break;
    case GUI_BUTTON:
      if (def.eventID == _heldID) {
        if (def.eventID == _hoverID) {
          drawRec(def, _theme.colorButtonPressed);
          bstate = BSTATE_PRESSED;
        } else {
          drawRec(def, _theme.colorButtonHeldOnly);
          bstate = BSTATE_HELD_ONLY;
        }
      } else if (def.eventID == _hoverID && !_heldID) {
        drawRec(def, _theme.colorButtonHover);
        bstate = BSTATE_HOVER;
      } else {
        drawRec(def, _theme.colorButtonNormal);
        bstate = BSTATE_NORMAL;
      }
      drawElem(def.elems[0], bstate);
      break;
    case GUI_MENU:
      if (def.eventID == _heldID) {
        drawRec(def, _theme.colorMenuSelect);
        bstate = BSTATE_HELD_ONLY;
      } else if (def.eventID == _hoverID && !_heldID) {
        drawRec(def, _theme.colorMenuHover);
        bstate = BSTATE_HOVER;
      } else {
        drawRec(def, _theme.colorMenuNormal);
        bstate = BSTATE_NORMAL;
      }
      drawElem(def.elems[0], bstate);
      if (def._active) { drawElem(def.elems[1], bstate); }
      break;
    case GUI_MENU_ITEM:
      if (def.eventID == _hoverID) {
        drawRec(def, _theme.colorMenuItemSelect);
      }
      drawElem(def.elems[0], bstate);
      break;
    default:
      LOG_ERROR("unknown type ", def.type);
      break;
  }
}

gx::GuiElem* gx::Gui::findEventElem(float x, float y)
{
  // full search through all elems needed because of popup elements
  std::vector<GuiElem*> stack;
  GuiElem* e = &_rootElem;
  while (e->eventID == 0 || !contains(*e, x, y)) {
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

void gx::Gui::drawRec(const GuiElem& def, uint32_t col)
{
  if (col != 0) {
    _dl.color(col);
    _dl.rectangle(def._x, def._y, def._w, def._h);
  }

  if (_theme.colorEdge != 0) {
    _dl.color(_theme.colorEdge);
    _dl.rectangle(def._x, def._y, def._w, 1);
    _dl.rectangle(def._x, def._y + def._h - 1, def._w, 1);
    _dl.rectangle(def._x, def._y, 1, def._h);
    _dl.rectangle(def._x + def._w - 1, def._y, 1, def._h);
  }
}
