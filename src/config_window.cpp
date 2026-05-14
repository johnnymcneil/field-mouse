#include "field_mouse/config_window.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QEvent>
#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>

#include "ui_config_window.h"

#include "field_mouse/app_state.h"
#include "field_mouse/app_windows.h"
#include "field_mouse/input_tester_window.h"
#include "field_mouse/settings.h"

namespace FieldMouse {
ConfigWindow::ConfigWindow() {
  setAttribute(Qt::WA_QuitOnClose, false);
  setMinimumSize(760, 620);
  resize(820, 660);
  setWindowFlag(Qt::WindowMinimizeButtonHint, true);
  setWindowFlag(Qt::WindowCloseButtonHint, true);

  buildUi();

  inputTesterWindow_ = std::make_unique<InputTesterWindow>();

  syncFromSettings();
}

ConfigWindow::~ConfigWindow() = default;

void ConfigWindow::syncFromSettings() {
  if (remapToggle_) {
    const QSignalBlocker blocker(remapToggle_);
    remapToggle_->setChecked(g_app.settings.remapEnabled);
  }

  if (autostartToggle_) {
    const QSignalBlocker blocker(autostartToggle_);
    autostartToggle_->setChecked(g_app.settings.startWithWindows);
  }

  for (size_t i = 0; i < mapCombos_.size(); ++i) {
    if (!mapCombos_[i]) {
      continue;
    }
    const QSignalBlocker blocker(mapCombos_[i]);
    mapCombos_[i]->setCurrentIndex(static_cast<int>(ToIndex(g_app.settings.mapping[i])));
  }

  updateWindowTitle();
}

void ConfigWindow::showAndActivate() {
  showNormal();
  raise();
  activateWindow();
}

void ConfigWindow::handleInputTesterEvent(const MouseInputEvent& event) {
  if (inputTesterWindow_) {
    inputTesterWindow_->handleInputTesterEvent(event);
  }
}

void ConfigWindow::changeEvent(QEvent* event) {
  if (event->type() == QEvent::WindowStateChange && isMinimized()) {
    if (inputTesterWindow_) {
      inputTesterWindow_->hide();
    }
    HideConfigWindowToTray();
  }
  QWidget::changeEvent(event);
}

void ConfigWindow::closeEvent(QCloseEvent* event) {
  if (inputTesterWindow_) {
    inputTesterWindow_->hide();
  }
  HideConfigWindowToTray();
  event->ignore();
}

void ConfigWindow::applyStyles() {
  QFile styleFile(QStringLiteral(":/styles/config_window.qss"));
  if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    setStyleSheet({});
    return;
  }

  setStyleSheet(QString::fromUtf8(styleFile.readAll()));
}

void ConfigWindow::bindGeneratedWidgets() {
  statusPill_ = ui_->statusPillLabel;
  remapToggle_ = ui_->remapToggleCheckBox;
  autostartToggle_ = ui_->autostartToggleCheckBox;
  openInputTesterButton_ = ui_->openInputTesterButton;

  mapCombos_ = {
    ui_->mapLeftCombo,
    ui_->mapRightCombo,
    ui_->mapMiddleCombo,
    ui_->mapX1Combo,
    ui_->mapX2Combo,
  };
}

void ConfigWindow::assignSemanticObjectNames() {
  for (QFrame* frame : {ui_->heroCardFrame, ui_->behaviorCardFrame, ui_->mappingsCardFrame}) {
    frame->setObjectName(QStringLiteral("card"));
  }

  ui_->heroTitleLabel->setObjectName(QStringLiteral("heroTitle"));
  ui_->heroBodyLabel->setObjectName(QStringLiteral("heroBody"));
  statusPill_->setObjectName(QStringLiteral("pill"));

  for (QLabel* label : {
         ui_->behaviorTitleLabel,
         ui_->mappingsTitleLabel,
       }) {
    label->setObjectName(QStringLiteral("cardTitle"));
  }

  for (QLabel* label : {
         ui_->behaviorSubtitleLabel,
         ui_->mappingsSubtitleLabel,
       }) {
    label->setObjectName(QStringLiteral("cardSubtitle"));
  }

  for (QLabel* label : {
         ui_->mapLeftLabel,
         ui_->mapRightLabel,
         ui_->mapMiddleLabel,
         ui_->mapX1Label,
         ui_->mapX2Label,
       }) {
    label->setObjectName(QStringLiteral("mapLabel"));
  }
}

void ConfigWindow::populateMappingCombos() {
  for (QComboBox* combo : mapCombos_) {
    combo->clear();
    for (size_t i = 0; i < static_cast<size_t>(MouseButton::Count); ++i) {
      combo->addItem(QString::fromWCharArray(ButtonDisplayName(static_cast<MouseButton>(i))));
    }
  }
}

void ConfigWindow::connectSignals() {
  QObject::connect(remapToggle_, &QCheckBox::toggled, this, [this](bool checked) {
    handleRemapToggled(checked);
  });
  QObject::connect(autostartToggle_, &QCheckBox::toggled, this, [this](bool checked) {
    handleAutostartToggled(checked);
  });
  QObject::connect(openInputTesterButton_, &QPushButton::clicked, this, [this]() {
    handleOpenInputTester();
  });

  for (size_t i = 0; i < mapCombos_.size(); ++i) {
    QObject::connect(mapCombos_[i], qOverload<int>(&QComboBox::currentIndexChanged), this, [this, i](int index) {
      handleMappingChanged(i, index);
    });
  }
}

void ConfigWindow::buildUi() {
  ui_ = std::make_unique<Ui::ConfigWindow>();
  ui_->setupUi(this);

  bindGeneratedWidgets();
  assignSemanticObjectNames();
  populateMappingCombos();
  applyStyles();
  connectSignals();
}

void ConfigWindow::handleAutostartToggled(bool checked) {
  if (!SetAutostartEnabledInRegistry(checked)) {
    QMessageBox::critical(this, QStringLiteral("Field Mouse"), QStringLiteral("Failed to update startup setting."));
    syncFromSettings();
    return;
  }

  g_app.settings.startWithWindows = checked;
  SaveSettings();
  SyncUiFromSettings();
}

void ConfigWindow::handleMappingChanged(size_t index, int selectedIndex) {
  if (selectedIndex < 0 || selectedIndex >= static_cast<int>(MouseButton::Count)) {
    return;
  }

  g_app.settings.mapping[index] = static_cast<MouseButton>(selectedIndex);
  SaveSettings();
}

void ConfigWindow::handleOpenInputTester() {
  if (inputTesterWindow_) {
    inputTesterWindow_->setWindowIcon(windowIcon());
    inputTesterWindow_->showAndActivate();
  }
}

void ConfigWindow::handleRemapToggled(bool checked) {
  g_app.settings.remapEnabled = checked;
  SaveSettings();
  SyncUiFromSettings();
}

void ConfigWindow::updateWindowTitle() {
  const bool enabled = g_app.settings.remapEnabled;
  setWindowTitle(QStringLiteral("Field Mouse - %1").arg(enabled ? QStringLiteral("Remap ON") : QStringLiteral("Remap OFF")));
  if (statusPill_) {
    statusPill_->setText(enabled ? QStringLiteral("REMAP ACTIVE") : QStringLiteral("REMAP OFF"));
    statusPill_->setStyleSheet(
      enabled
        ? QStringLiteral("QLabel#pill { background: #244538; color: #b7f5bf; border-radius: 999px; padding: 6px 12px; font-weight: 700; }")
        : QStringLiteral("QLabel#pill { background: #463126; color: #ffd7b2; border-radius: 999px; padding: 6px 12px; font-weight: 700; }")
    );
  }
}

} // namespace FieldMouse