#include "mbed.h"
#include "cvgui.h"

namespace cv
{

  Widget::Widget(Frame *frame_, int flags_)
    : frame(frame_), flags(flags_)
  {
  }

  Widget::~Widget()
  {
  }

  bool Widget::requestRedraw() const
  {
    return false;
  }

  void Widget::setFocus(bool is_focus)
  {
    if(flags & WIDGET_CAN_FOCUS)
    {
      if(is_focus)
      {
        flags |= (uint16_t) WIDGET_FOCUSED;
      }
      else
      {
        flags &= ~uint16_t(WIDGET_FOCUSED);
      }
    }
  }

  bool Widget::hasFocus() const
  {
    return (flags & WIDGET_FOCUSED) != 0;
  }

  void Widget::activate()
  {
    if(flags & WIDGET_CAN_ACTIVATE)
    {
      flags |= (uint16_t) WIDGET_ACTIVATED;
    }
  }

  void Widget::dectivate()
  {
      flags &= ~((uint16_t) WIDGET_ACTIVATED);
  }

  bool Widget::isActive() const
  {
    return (flags & (uint16_t) WIDGET_ACTIVATED) != 0;
  }

  bool Widget::isAnimated() const
  {
    return (flags & (uint16_t) WIDGET_ANIMATED) != 0;
  }

  void Widget::show()
  {
      flags |= (uint16_t) WIDGET_VISIBLE;
  }

  void Widget::hide()
  {
      flags &= ~((uint16_t) WIDGET_VISIBLE);
  }

  bool Widget::isVisible() const
  {
    return (flags & (uint16_t) WIDGET_VISIBLE) != 0;
  }

  void Widget::enable()
  {
      flags |= (uint16_t) WIDGET_ENABLED;
  }

  void Widget::disable()
  {
      flags &= ~((uint16_t) WIDGET_ENABLED);
  }

  bool Widget::isEnabled() const
  {
    return (flags & (uint16_t) WIDGET_ENABLED) != 0;
  }

  Line::Line(Frame *frame_)
    : Widget(frame_, WIDGET_VISIBLE)
  {
  }

  void Line::draw(Painter& painter_)
  {
    cv::Point pt1(x, y), pt2(x + width - 1, y + height - 1);
    if(crossed_line)
    {
      std::swap(pt1.x, pt2.x);
    }
    painter_.line(pt1, pt2, fg_color, thickness);
  }

  Label::Label(Frame *frame_)
    : Widget(frame_, WIDGET_VISIBLE)
  {
  }

  void Label::draw(Painter& painter_)
  {
    if((flags & WIDGET_VISIBLE) == 0)
    {
      return;
    }
    painter_.rectangle(cv::Point(x, y), cv::Point(x + width, y + height), bg_color, -1);
    if(font != nullptr && !text.empty())
    {
      cv::Size text_size = font->get_text_size(text);
      int text_x = x + width / 2 - text_size.width / 2;
      int text_y = y + height / 2 - text_size.height / 2;
      painter_.putText(text, cv::Point(text_x, text_y), *font, fg_color, fg_color);
    }    
  }

  MultiPageLabel::MultiPageLabel(Frame *frame_)
    : Widget(frame_, WIDGET_VISIBLE)
  {
  }

  void MultiPageLabel::set_text(const std::string& text_)
  {
    text = text_;
    page_char_count.clear();
    page = 0;
  }

  const std::string& MultiPageLabel::get_text() const
  {
    return text;
  }

  void MultiPageLabel::draw(Painter& painter_)
  {
    if((flags & WIDGET_VISIBLE) == 0)
    {
      return;
    }
    if(font != nullptr && !text.empty())
    {
      size_t text_start = 0;
      size_t consumed_chars = 0;
      for(size_t current_page = 0; current_page <= page; current_page++)
      {
        if(current_page == page || current_page >= page_char_count.size() || text_start + page_char_count[current_page] >= text.size())
        {
          painter_.rectangle(cv::Point(x, y), cv::Point(x + width, y + height), bg_color, -1);
          painter_.putText(text.substr(text_start), cv::Point(x, y), *font, fg_color, fg_color, width, &consumed_chars);
          if(current_page >= page_char_count.size())
          {
            page_char_count.push_back(consumed_chars);
          }
          if(text_start + page_char_count[current_page] >= text.size())
          {
            page = current_page;
            break;
          }
        }
        else
        {
          text_start += page_char_count[current_page];
        }
      }
    }
  }

  Image::Image(Frame *frame_)
    : Widget(frame_, WIDGET_VISIBLE)
  {
  }

  void Image::draw(Painter& painter_)
  {
    if((flags & WIDGET_VISIBLE) == 0 || image.empty())
    {
      return;
    }
    if(has_alpha)
    {
      painter_.drawBitmapWithAlpha(image, cv::Point(x, y));
    }
    else
    {
      painter_.drawBitmap(image, cv::Point(x, y));
    }
  }
  
  Animation::Animation(Frame *frame_)
    : Widget(frame_, WIDGET_ENABLED|WIDGET_VISIBLE|WIDGET_ANIMATED)
  {
  }

  void Animation::draw(Painter& painter_)
  {
    if((flags & WIDGET_VISIBLE) == 0 || image.empty())
    {
      return;
    }
    int frame_count = anime_rows * anime_cols;
    int frame_rows = image.rows / anime_rows;
    int frame_cols = image.cols / anime_cols;
    if(current_frame < 0)
    {
      current_frame = 0;
      update_time = Kernel::Clock::now();
    }
    else
    {
      auto now = Kernel::Clock::now();
      if(now - update_time >= update_interval)
      {
        if(++current_frame >= frame_count)
        {
          current_frame = 0;
        }
        update_time = now;
      }
    }
    int current_frame_row = current_frame / frame_cols;
    int current_frame_col = current_frame % frame_cols;
    cv::Rect current_frame_rc = cv::Rect(current_frame_col * frame_cols, current_frame_row * frame_rows, frame_cols, frame_rows)
      & cv::Rect(0, 0, image.cols, image.rows);
    if(!current_frame_rc.empty())
    {
      if(has_alpha)
      {
        painter_.drawBitmapWithAlpha(image(current_frame_rc), cv::Point(x, y));
      }
      else
      {
        painter_.drawBitmap(image(current_frame_rc), cv::Point(x, y));
      }    
    }
  }

  bool Animation::requestRedraw() const
  {
    if((flags & WIDGET_VISIBLE) == 0)
    {
      return false;
    }
    if(current_frame < 0)
    {
      return true;
    }
    if(anime_rows * anime_cols <= 1)
    {
      return false;
    }
    auto now = Kernel::Clock::now();
    return now - update_time >= update_interval;
  }

  Button::Button(Frame *frame_)
    : Widget(frame_, WIDGET_VISIBLE|WIDGET_ENABLED|WIDGET_CAN_FOCUS|WIDGET_CAN_ACTIVATE)
  {
  }

  void Button::draw(Painter& painter_)
  {
    if((flags & WIDGET_VISIBLE) == 0)
    {
      return;
    }
    // draw background
    if(isActive())
    {
      painter_.rectangle(cv::Point(x + 1, y + 1), cv::Point(x + width - 1, y + height - 1), ~bg_color, -1);
    }
    else
    {
      painter_.rectangle(cv::Point(x + 1, y + 1), cv::Point(x + width - 1, y + height - 1), bg_color, -1);
    }
    // draw border
    painter_.rectangle(cv::Point(x, y), cv::Point(x + width, y + height), border_color, hasFocus() ? 2: 1);
    // draw text
    if(font != nullptr && !text.empty())
    {
      cv::Size text_size = font->get_text_size(text);
      int text_x = x + width / 2 - text_size.width / 2;
      int text_y = y + height / 2 - text_size.height / 2;
      painter_.putText(text, cv::Point(text_x, text_y), *font, fg_color, bg_color);
    }
  }

  void CheckBox::draw(Painter& painter_)
  {
  }

  Frame::Frame(Painter& painter_, int16_t offset_x_, int16_t offset_y_ )
    : painter(painter_), offset_x(offset_x_), offset_y(offset_y_)
  {
  }

  Widget* Frame::addWidget(std::unique_ptr<Widget>&& widget_)
  {
    widgets.push_back(std::move(widget_));
    int16_t widget_index = int16_t(widgets.size()) - 1;
    auto& widget = *widgets[widget_index];
    widget.index = widget_index;
    if(focus_index < 0 && (widget.flags & WIDGET_CAN_FOCUS))
    {
      widget.setFocus(true);
      focus_index = widget_index;
    }
    return &widget;
  }

  void Frame::redraw()
  {
    if(bg_image.empty())
    {
      painter.fill(bg_color);
    }
    else
    {
      painter.drawBitmap(bg_image, cv::Point(0, 0));
    }
    for(const auto& widget: widgets)
    {
      widget->draw(painter);
    }
  }

  void Frame::redrawBackground(cv::Rect rc)
  {
    cv::Rect safe_rc = rc & cv::Rect(cv::Point(0, 0), painter.get_mat_size());
    if(!safe_rc.empty())
    {
      if(bg_image.empty())
      {
        painter.rectangle(safe_rc.tl(), safe_rc.br(), bg_color, -1);
      }
      else
      {
        safe_rc &= cv::Rect(0, 0, bg_image.cols, bg_image.rows);
        if(!safe_rc.empty())
        {
          painter.drawBitmap(bg_image(safe_rc), safe_rc.tl());
        }
      }
    }
  }

  void Frame::redrawRect(cv::Rect rc)
  {
    cv::Rect safe_rc = rc & cv::Rect(cv::Point(0, 0), painter.get_mat_size());
    redrawBackground(safe_rc);
    for(const auto& widget: widgets)
    {
      cv::Rect overlap_rc = safe_rc & cv::Rect(widget->x, widget->y, widget->width, widget->height);
      if(!overlap_rc.empty())
      {
        widget->draw(painter);
      }
    }
  }

  void Frame::redrawWidget(Widget *widget)
  {
    if(widget->frame == this)
    {
      redrawRect(cv::Rect(widget->x, widget->y, widget->width, widget->height));
    }
  }

  void Frame::redrawWidget(int widget_index)
  {
    redrawWidget(widgets[widget_index].get());
  }

  size_t Frame::getWidgetCount() const
  {
    return widgets.size();
  }

  Widget *Frame::getWidget(size_t index)
  {
    if(index < widgets.size())
    {
      return widgets[index].get();
    }
    return nullptr;
  }

  Widget *Frame::getWidgetAt(cv::Point pt)
  {
    for(auto& widget: widgets)
    {
      if(pt.x >= widget->x && pt.x < widget->x + widget->width &&
        pt.y >= widget->y && pt.y < widget->y + widget->height)
      {
        return widget.get();
      }
    }
    return nullptr;
  }

  void Frame::setFocusedWdiget(Widget *widget)
  {
    if(widget->frame != this)
    {
      return;
    }
    if(focus_index >= 0)
    {
      widgets[focus_index]->setFocus(false);
      redrawWidget(focus_index);
    }
    focus_index = widget->index;
    widget->setFocus(true);
    redrawWidget(widget);
  }

  Widget *Frame::getFocusedWidget()
  {
    if(focus_index >= 0)
    {
      Widget *focused_widget = widgets[focus_index].get();
      if(focused_widget->isEnabled())
      {
        return focused_widget;
      }
    }
    return nullptr;
  }

  Widget *Frame::getPreviousWidget(Widget *widget)
  {
    if(widget != nullptr)
    {
      int16_t index = widget->index - 1;
      if(index < 0)
      {
        index = int(widgets.size()) - 1;
      }
      return widgets[index].get();
    }
    return nullptr;
  }

  Widget *Frame::getNextWidget(Widget *widget)
  {
    if(widget != nullptr)
    {
      int16_t index = widget->index + 1;
      if(index >= int(widgets.size()))
      {
        index = 0;
      }
      return widgets[index].get();
    }
    return nullptr;
  }

}
