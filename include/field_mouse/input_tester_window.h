#ifndef FIELD_MOUSE_INPUT_TESTER_WINDOW_H
#define FIELD_MOUSE_INPUT_TESTER_WINDOW_H

#include <QWidget>

#include <array>
#include <memory>

#include "field_mouse/app_types.h"

class QLabel;
class QPlainTextEdit;
class QTimer;

namespace Ui {
class InputTesterWindow;
}

namespace FieldMouse {

class InputTesterWindow : public QWidget {
public:
  explicit InputTesterWindow(QWidget* parent = nullptr);
  ~InputTesterWindow() override;

  void showAndActivate();
  void handleInputTesterEvent(const MouseInputEvent& event);

protected:
  void closeEvent(QCloseEvent* event) override;

private:
  void applyStyles();
  void buildUi();
  void bindGeneratedWidgets();
  void assignSemanticObjectNames();
  void initializeInputTesterLights();
  void clearInputTesterLights();
  void setTesterLightState(InputTesterLight light);
  void updateInputTesterText(const MouseInputEvent& event);

  std::array<QLabel*, static_cast<size_t>(InputTesterLight::Count)> inputTesterLights_ = {};
  QPlainTextEdit* inputTesterLog_ = nullptr;
  QTimer* clearTimer_ = nullptr;
  std::unique_ptr<Ui::InputTesterWindow> ui_;
};

} // namespace FieldMouse

#endif
