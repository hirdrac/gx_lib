//
// gx/Gui.cc
// Copyright (C) 2020 Richard Bradley
//

#include "Gui.hh"
#include "Window.hh"
#include "Font.hh"
#include "print.hh"
#include "Logger.hh"
#include <algorithm>
#include <cassert>

namespace {
  constexpr bool contains(const gx::GuiElem& e, float x, float y)
  {
    return (x >= e._x) && (x < (e._x + e._w))
      && (y >= e._y) && (y < (e._y + e._h));
  }
}


gx::Gui::Gui(const GuiElem& rootElem) : _rootElem(rootElem) { }
gx::Gui::Gui(GuiElem&& rootElem) : _rootElem(std::move(rootElem)) { }

void gx::Gui::layout(const GuiTheme& theme, float x, float y, AlignEnum align)
{
  _theme = theme;
  _pt.set(x, y);
  _align = align;
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
    // FIXME - adjust _x/_y based on alignment?
    calcPos(_rootElem, _pt.x, _pt.y);
    _needPos = false;
  }

  // button event & state update
  bool held = win.buttons() & BUTTON1;
  GuiElem* ptr = findEventElem(win.mouseX(), win.mouseY());
  int id = ptr ? ptr->eventID : 0;
  if (_buttonHoverID != id) {
    _buttonHoverID = id;
    _needRender = true;
  }

  if (held && (win.events() & EVENT_MOUSE_BUTTON)) {
    _buttonPressedID = id;
    _lastPressedID = id;
    _needRender = true;
  } else {
    _buttonPressedID = 0;
  }

  if (!held && _lastPressedID == id) {
    _buttonReleasedID = id;
    _lastPressedID = 0;
  } else {
    if (_buttonReleasedID) { _needRender = true; }
    _buttonReleasedID = 0;
  }

  if (held) {
    _buttonHeldID = _lastPressedID;
  } else {
    _buttonHeldID = 0;
    if (_lastPressedID) { _needRender = true; }
    _lastPressedID = 0;
  }

  if (contains(_rootElem, win.mouseX(), win.mouseY())) {
    win.consumeEvent(gx::EVENT_MOUSE_BUTTON);
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

void gx::Gui::calcSize(GuiElem& def)
{
  switch (def.type) {
    case GUI_HFRAME: {
      float total_w = 0, max_h = 0;
      for (GuiElem& e : def.elems) {
        calcSize(e);
        total_w += e._w + _theme.border;
        max_h = std::max(max_h, e._h);
      }
      def._w = total_w + _theme.border;
      def._h = max_h + (_theme.border * 2);
      break;
    }
    case GUI_VFRAME: {
      float total_h = 0, max_w = 0;
      for (GuiElem& e : def.elems) {
        calcSize(e);
        total_h += e._h + _theme.border;
        max_w = std::max(max_w, e._w);
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
    case GUI_BUTTON: {
      assert(def.elems.size() == 1);
      GuiElem& e = def.elems[0];
      calcSize(e);
      def._w = e._w + (_theme.border * 2);
      def._h = e._h + (_theme.border * 2);
      break;
    }
    default:
      LOG_ERROR("unknown type");
      break;
  }
}

void gx::Gui::calcPos(GuiElem& def, float base_x, float base_y)
{
  def._x = base_x;
  def._y = base_y;

  switch (def.type) {
    case GUI_HFRAME:
      for (GuiElem& e : def.elems) {
        base_x += _theme.border;
        float yy = 0;
        if ((e.align & ALIGN_VCENTER) == ALIGN_VCENTER) {
          yy = (def._h - e._h) / 2.0f;
        } else if (e.align & ALIGN_BOTTOM) {
          yy = (def._h - e._h) - _theme.border;
        } else {
          yy = _theme.border;
        }
        calcPos(e, base_x, base_y + yy);
        base_x += e._w;
      }
      break;
    case GUI_VFRAME:
      for (GuiElem& e : def.elems) {
        base_y += _theme.border;
        float xx = 0;
        if ((e.align & ALIGN_HCENTER) == ALIGN_HCENTER) {
          xx = (def._w - e._w) / 2.0f;
        } else if (e.align & ALIGN_RIGHT) {
          xx = (def._w - e._w) - _theme.border;
        } else {
          xx = _theme.border;
        }
        calcPos(e, base_x + xx, base_y);
        base_y += e._h;
      }
      break;
    case GUI_BUTTON:
      calcPos(def.elems[0], base_x + _theme.border, base_y + _theme.border);
      break;
    default:
      break;
  }
}

void gx::Gui::drawElem(GuiElem& def, ButtonState bstate)
{
  // FIXME - add style option to HFRAME,VFRAME,BUTTON

  switch (def.type) {
    case GUI_HFRAME:
      //drawRec(def, packRGBA8(.1,.1,.2,1));
      break;
    case GUI_VFRAME:
      //drawRec(def, packRGBA8(.1,.2,.1,1));
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
      if (def.eventID == _buttonHeldID) {
        if (def.eventID == _buttonHoverID) {
          drawRec(def, _theme.colorButtonPressed);
          bstate = BSTATE_PRESSED;
        } else {
          drawRec(def, _theme.colorButtonHeldOnly);
          bstate = BSTATE_HELD_ONLY;
        }
      } else if (def.eventID == _buttonHoverID && !_buttonHeldID) {
        drawRec(def, _theme.colorButtonHover);
        bstate = BSTATE_HOVER;
      } else {
        drawRec(def, _theme.colorButtonNormal);
        bstate = BSTATE_NORMAL;
      }
      break;
    default:
      LOG_ERROR("unknown type");
      break;
  }

  for (GuiElem& e : def.elems) {
    drawElem(e, bstate);
  }
}

gx::GuiElem* gx::Gui::findEventElem(float x, float y)
{
  gx::GuiElem* ptr = &_rootElem;
  gx::GuiElem* end = ptr + 1;
  while (ptr != end) {
    GuiElem& e = *ptr;
    if (!contains(e, x, y)) {
      ++ptr;
      continue;
    }

    if (e.eventID > 0) {
      return ptr;
    }

    ptr = e.elems.data();
    end = ptr + e.elems.size();
  }

  return nullptr;
}

gx::GuiElem* gx::Gui::findElem(int eventID)
{
  std::vector stack = { &_rootElem };
  while (!stack.empty()) {
    auto e = stack.back();
    if (e->eventID == eventID) {
      return e;
    }

    stack.pop_back();
    for (auto& x : e->elems) {
      stack.push_back(&x);
    }
  }

  return nullptr;
}

void gx::Gui::drawRec(const GuiElem& def, uint32_t col)
{
  if (col != 0) {
    _dl.color(col);
    _dl.rectangle(def._x, def._y, def._w, def._h);
  }

  if (_theme.colorFrame != 0) {
    _dl.color(_theme.colorFrame);
    _dl.rectangle(def._x, def._y, def._w, 1);
    _dl.rectangle(def._x, def._y + def._h - 1, def._w, 1);
    _dl.rectangle(def._x, def._y, 1, def._h);
    _dl.rectangle(def._x + def._w - 1, def._y, 1, def._h);
  }
}
