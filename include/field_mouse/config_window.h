#ifndef FIELD_MOUSE_CONFIG_WINDOW_H
#define FIELD_MOUSE_CONFIG_WINDOW_H

#include <QWidget>

#include <array>
#include <memory>

#include "field_mouse/app_types.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;

namespace Ui {
class ConfigWindow;
}

namespace FieldMouse {

class InputTesterWindow;

class ConfigWindow : public QWidget {
public:
  ConfigWindow();
  ~ConfigWindow() override;

  void syncFromSettings();
  void showAndActivate();
  void handleInputTesterEvent(const MouseInputEvent& event);

protected:
  void changeEvent(QEvent* event) override;
  void closeEvent(QCloseEvent* event) override;

private:
  void applyStyles();
  void buildUi();
  void bindGeneratedWidgets();
  void assignSemanticObjectNames();
  void populateMappingCombos();
  void connectSignals();
  void handleAutostartToggled(bool checked);
  void handleMappingChanged(size_t index, int selectedIndex);
  void handleOpenInputTester();
  void handleRemapToggled(bool checked);
  void updateWindowTitle();

  QLabel* statusPill_ = nullptr;
  QCheckBox* remapToggle_ = nullptr;
  QCheckBox* autostartToggle_ = nullptr;
  QPushButton* openInputTesterButton_ = nullptr;
  std::array<QComboBox*, static_cast<size_t>(MouseButton::Count)> mapCombos_ = {};
  std::unique_ptr<InputTesterWindow> inputTesterWindow_;
  std::unique_ptr<Ui::ConfigWindow> ui_;
};

} // namespace FieldMouse

#endif