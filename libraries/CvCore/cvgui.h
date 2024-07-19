#pragma once

#include <mbed.h>
#include <string>
#include <vector>
#include <memory>
#include <EventQueue.h>
#include "cvimgproc.h"
#include "tinyfsm.h"

namespace cv
{

  enum { WIDGET_VISIBLE = 1, WIDGET_ENABLED = 2, WIDGET_CAN_FOCUS = 4, WIDGET_FOCUSED = 8, WIDGET_CAN_ACTIVATE = 16, WIDGET_ACTIVATED = 32, WIDGET_ANIMATED = 64 };

  class Frame;

  class Widget
  {
  public:
    friend class Frame;
    int16_t x = 0, y = 0, width = 0, height = 0;
    uint16_t bg_color = 0, fg_color = 0xffff, border_color = 0;
    int16_t index = 0;
    uint32_t user_data = 0;

  protected:
    Frame * frame;
    uint16_t flags = 0;

  public:
    Widget(Frame *frame_, int flags_);

    virtual ~Widget();

    virtual void draw(Painter& painter_) = 0;

    virtual bool requestRedraw() const;

    bool hasFocus() const;

    void activate();

    void dectivate();

    bool isActive() const;

    bool isAnimated() const;

    void show();

    void hide();

    bool isVisible() const;

    void enable();

    void disable();

    bool isEnabled() const;

  protected:
      void setFocus(bool is_focus);
  };

  class Line : public Widget
  {
  public:
    Line(Frame *frame_);

    uint16_t thickness = 1;
    bool crossed_line = false;

  	void draw(Painter& painter_) override;
  };

  class Label : public Widget
  {
  public:
    Label(Frame *frame_);

  	std::string text;
    cv::FontBase *font = nullptr;
  
  	void draw(Painter& painter_) override;
  };

  class MultiPageLabel : public Widget
  {
  public:
    MultiPageLabel(Frame *frame_);

    cv::FontBase *font = nullptr;
    size_t page = 0;

    void set_text(const std::string& text_);

    const std::string& get_text() const;

  	void draw(Painter& painter_) override;

  private:
  	std::string text;
    std::vector<size_t> page_char_count;
  };

  class Image : public Widget
  {
  public:
    Image(Frame *frame_);

    cv::Mat image;
    bool has_alpha = false;

  	void draw(Painter& painter_) override;
  };

  class Animation : public Widget
  {
  public:
    Animation(Frame *frame_);

    cv::Mat image;
    uint16_t anime_rows = 1;
    uint16_t anime_cols = 1;
    int16_t current_frame = -1;
    bool has_alpha = false;
    std::chrono::milliseconds update_interval = 250ms;
    Kernel::Clock::time_point update_time;

  	void draw(Painter& painter_) override;

    bool requestRedraw() const override;
  };

  class Button : public Widget
  {
  public:
  	std::string text;
    cv::FontBase *font = nullptr;
    uint16_t active_color;
    bool active_state = false;

    Button(Frame *frame_);

  	void draw(Painter& painter_) override;
  };

  class CheckBox : public Button
  {
  public:
  	void draw(Painter& painter_) override;
  };

  class Frame
  {
  public:
    Frame(Painter& painter_, int16_t offset_x_ = 0, int16_t offset_y_ = 0);

    Widget* addWidget(std::unique_ptr<Widget>&& widget);

    template<typename WidgetType>
    WidgetType* addWidget(int16_t x = 0, int16_t y = 0, int16_t width = 0, int16_t height = 0)
    {
      WidgetType* widget = static_cast<WidgetType*>(addWidget(std::make_unique<WidgetType>(this)));
      widget->x = x;
      widget->y = y;
      widget->width = width;
      widget->height = height;
      return widget;
    }

    template<typename WidgetType>
    WidgetType* getWidget(int index)
    {
      return static_cast<WidgetType*>(widgets[index].get());
    }

    void redraw();

    void redrawBackground(cv::Rect rc);

    void redrawRect(cv::Rect rc);

    void redrawWidget(Widget *widget);

    void redrawWidget(int widget_index);

    size_t getWidgetCount() const;

    Widget *getWidget(size_t index);

    Widget *getWidgetAt(cv::Point pt);

    void setFocusedWdiget(Widget *widget);

    Widget *getFocusedWidget();

    Widget *getPreviousWidget(Widget *widget);

    Widget *getNextWidget(Widget *widget);

    cv::Mat bg_image;
    uint16_t bg_color = 0;
    int16_t focus_index = -1;
    std::vector<std::unique_ptr<Widget>> widgets;

  private:
    Painter& painter;

  public:
    int16_t offset_x = 0, offset_y = 0;
  };

}
