#ifndef FIELD_MOUSE_CONFIG_WINDOW_H
#define FIELD_MOUSE_CONFIG_WINDOW_H

#include <QWidget>

#include <array>

#include "field_mouse/app_types.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QTimer;
class QVBoxLayout;

namespace FieldMouse {

class ConfigWindow : public QWidget {
public:
  ConfigWindow();

  void syncFromSettings();
  void showAndActivate();
  void handleInputTesterEvent(const MouseInputEvent& event);

protected:
  void changeEvent(QEvent* event) override;
  void closeEvent(QCloseEvent* event) override;

private:
  void applyStyles();
  void buildUi();
  void buildHeroSection(QVBoxLayout* root);
  void buildBehaviorSection(QVBoxLayout* root);
  void buildMappingsSection(QVBoxLayout* root);
  void buildInputTesterSection(QVBoxLayout* root);
  void connectSignals();
  void clearInputTesterLights();
  void handleAutostartToggled(bool checked);
  void handleMappingChanged(size_t index, int selectedIndex);
  void handleRemapToggled(bool checked);
  void setTesterLightState(InputTesterLight light);
  void updateInputTesterText(const MouseInputEvent& event);
  void updateWindowTitle();

  QLabel* statusPill_ = nullptr;
  QCheckBox* remapToggle_ = nullptr;
  QCheckBox* autostartToggle_ = nullptr;
  std::array<QComboBox*, static_cast<size_t>(MouseButton::Count)> mapCombos_ = {};
  std::array<QLabel*, static_cast<size_t>(InputTesterLight::Count)> inputTesterLights_ = {};
  QPlainTextEdit* inputTesterLog_ = nullptr;
  QTimer* clearTimer_ = nullptr;
};

} // namespace FieldMouse

#endif