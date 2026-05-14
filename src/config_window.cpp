#include "field_mouse/config_window.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>

#include <array>

#include "field_mouse/app_constants.h"
#include "field_mouse/app_state.h"
#include "field_mouse/app_windows.h"
#include "field_mouse/settings.h"

namespace FieldMouse {
namespace {

QString MouseMessageName(UINT message) {
  switch (message) {
  case WM_MOUSEMOVE:
    return QStringLiteral("Move");
  case WM_LBUTTONDOWN:
    return QStringLiteral("Left Down");
  case WM_LBUTTONUP:
    return QStringLiteral("Left Up");
  case WM_RBUTTONDOWN:
    return QStringLiteral("Right Down");
  case WM_RBUTTONUP:
    return QStringLiteral("Right Up");
  case WM_MBUTTONDOWN:
    return QStringLiteral("Middle Down");
  case WM_MBUTTONUP:
    return QStringLiteral("Middle Up");
  case WM_XBUTTONDOWN:
    return QStringLiteral("X Button Down");
  case WM_XBUTTONUP:
    return QStringLiteral("X Button Up");
  case WM_MOUSEWHEEL:
    return QStringLiteral("Mouse Wheel");
  case WM_MOUSEHWHEEL:
    return QStringLiteral("Mouse Horizontal Wheel");
  default:
    return QStringLiteral("Mouse Event");
  }
}

QString ButtonNameFromMouseData(UINT message, DWORD mouseData) {
  switch (message) {
  case WM_XBUTTONDOWN:
  case WM_XBUTTONUP:
    return HIWORD(mouseData) == XBUTTON2 ? QStringLiteral("X2") : QStringLiteral("X1");
  default:
    return {};
  }
}

InputTesterLight InputTesterLightFromMessage(UINT message, DWORD mouseData) {
  switch (message) {
  case WM_MOUSEMOVE:
    return InputTesterLight::Move;
  case WM_MOUSEWHEEL:
  case WM_MOUSEHWHEEL:
    return InputTesterLight::Wheel;
  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
    return InputTesterLight::Left;
  case WM_RBUTTONDOWN:
  case WM_RBUTTONUP:
    return InputTesterLight::Right;
  case WM_MBUTTONDOWN:
  case WM_MBUTTONUP:
    return InputTesterLight::Middle;
  case WM_XBUTTONDOWN:
  case WM_XBUTTONUP:
    return HIWORD(mouseData) == XBUTTON2 ? InputTesterLight::X2 : InputTesterLight::X1;
  default:
    return InputTesterLight::Any;
  }
}

QString FormatInputTesterText(const MouseInputEvent& event) {
  const bool appInjected = event.extraInfo == kInjectedSentinel;
  const bool injected = appInjected || (event.flags & (LLMHF_INJECTED | LLMHF_LOWER_IL_INJECTED)) != 0;
  const QString sourceName = MouseMessageName(event.message);
  const QString buttonName = ButtonNameFromMouseData(event.message, event.mouseData);
  const QString kind = injected ? QStringLiteral("injected") : QStringLiteral("regular");
  const QString suffix = appInjected ? QStringLiteral(", ours") : QString();

  if (event.message == WM_MOUSEWHEEL || event.message == WM_MOUSEHWHEEL) {
    const SHORT delta = static_cast<SHORT>(HIWORD(event.mouseData));
    return QStringLiteral("%1 %2 at (%3, %4) [%5%6]")
      .arg(sourceName)
      .arg(static_cast<int>(delta))
      .arg(event.x)
      .arg(event.y)
      .arg(kind)
      .arg(suffix);
  }

  if (!buttonName.isEmpty()) {
    return QStringLiteral("%1 %2 at (%3, %4) [%5%6]")
      .arg(sourceName)
      .arg(buttonName)
      .arg(event.x)
      .arg(event.y)
      .arg(kind)
      .arg(suffix);
  }

  return QStringLiteral("%1 at (%2, %3) [%4%5]")
    .arg(sourceName)
    .arg(event.x)
    .arg(event.y)
    .arg(kind)
    .arg(suffix);
}

QString LightStyle(bool active) {
  if (active) {
    return QStringLiteral(
      "QLabel {"
      " background-color: #8ddf74;"
      " color: #132015;"
      " border: 1px solid #5a9f49;"
      " border-radius: 10px;"
      " font-weight: 700;"
      " padding: 8px 12px;"
      "}"
    );
  }

  return QStringLiteral(
    "QLabel {"
    " background-color: #23303d;"
    " color: #c8d5e2;"
    " border: 1px solid #314454;"
    " border-radius: 10px;"
    " font-weight: 600;"
    " padding: 8px 12px;"
    "}"
  );
}

QFrame* MakeCard(const QString& title, const QString& subtitle, QWidget* parent, QVBoxLayout** bodyLayout) {
  auto* frame = new QFrame(parent);
  frame->setObjectName(QStringLiteral("card"));

  auto* layout = new QVBoxLayout(frame);
  layout->setContentsMargins(20, 18, 20, 18);
  layout->setSpacing(14);

  auto* titleLabel = new QLabel(title, frame);
  titleLabel->setObjectName(QStringLiteral("cardTitle"));
  auto* subtitleLabel = new QLabel(subtitle, frame);
  subtitleLabel->setObjectName(QStringLiteral("cardSubtitle"));
  subtitleLabel->setWordWrap(true);

  layout->addWidget(titleLabel);
  layout->addWidget(subtitleLabel);

  auto* contentLayout = new QVBoxLayout();
  contentLayout->setSpacing(12);
  layout->addLayout(contentLayout);
  *bodyLayout = contentLayout;
  return frame;
}

} // namespace

ConfigWindow::ConfigWindow() {
  setAttribute(Qt::WA_QuitOnClose, false);
  setMinimumSize(760, 620);
  resize(820, 660);
  setWindowFlag(Qt::WindowMinimizeButtonHint, true);
  setWindowFlag(Qt::WindowCloseButtonHint, true);

  buildUi();

  clearTimer_ = new QTimer(this);
  clearTimer_->setSingleShot(true);
  QObject::connect(clearTimer_, &QTimer::timeout, this, [this]() {
    clearInputTesterLights();
  });

  syncFromSettings();
}

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
  setTesterLightState(InputTesterLightFromMessage(event.message, event.mouseData));
  updateInputTesterText(event);
  clearTimer_->start(140);
}

void ConfigWindow::changeEvent(QEvent* event) {
  if (event->type() == QEvent::WindowStateChange && isMinimized()) {
    HideConfigWindowToTray();
  }
  QWidget::changeEvent(event);
}

void ConfigWindow::closeEvent(QCloseEvent* event) {
  HideConfigWindowToTray();
  event->ignore();
}

void ConfigWindow::buildUi() {
  setStyleSheet(
    QStringLiteral(
      "ConfigWindow { background: #0f1720; color: #e6edf3; }"
      "QFrame#card { background: #16212d; border: 1px solid #223244; border-radius: 18px; }"
      "QLabel#heroTitle { font-size: 28px; font-weight: 700; color: #f7fbff; }"
      "QLabel#heroBody { color: #9db0c2; font-size: 14px; }"
      "QLabel#pill { background: #244538; color: #b7f5bf; border-radius: 999px; padding: 6px 12px; font-weight: 700; }"
      "QLabel#cardTitle { font-size: 18px; font-weight: 700; color: #f0f6fc; }"
      "QLabel#cardSubtitle { color: #8ea4b9; }"
      "QCheckBox { spacing: 10px; font-size: 14px; color: #d8e2ec; }"
      "QComboBox, QPlainTextEdit { background: #0f1720; color: #eef5fb; border: 1px solid #314454; border-radius: 10px; padding: 8px 10px; }"
      "QComboBox::drop-down { width: 28px; border: none; }"
      "QComboBox QAbstractItemView { background: #16212d; color: #eef5fb; selection-background-color: #30506c; }"
      "QPlainTextEdit { selection-background-color: #30506c; }"
      "QLabel#mapLabel { color: #adc0d1; font-weight: 600; }"
    )
  );

  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(24, 24, 24, 24);
  root->setSpacing(18);

  auto* heroCard = new QFrame(this);
  heroCard->setObjectName(QStringLiteral("card"));
  auto* heroLayout = new QHBoxLayout(heroCard);
  heroLayout->setContentsMargins(22, 20, 22, 20);

  auto* heroTextLayout = new QVBoxLayout();
  heroTextLayout->setSpacing(6);
  auto* heroTitle = new QLabel(QStringLiteral("Field Mouse"), heroCard);
  heroTitle->setObjectName(QStringLiteral("heroTitle"));
  auto* heroBody = new QLabel(
    QStringLiteral("Remap mouse buttons, manage startup behavior, and watch live input events from a cleaner Qt front end."),
    heroCard
  );
  heroBody->setObjectName(QStringLiteral("heroBody"));
  heroBody->setWordWrap(true);
  heroTextLayout->addWidget(heroTitle);
  heroTextLayout->addWidget(heroBody);

  statusPill_ = new QLabel(heroCard);
  statusPill_->setObjectName(QStringLiteral("pill"));
  statusPill_->setAlignment(Qt::AlignCenter);
  statusPill_->setMinimumWidth(140);

  heroLayout->addLayout(heroTextLayout, 1);
  heroLayout->addWidget(statusPill_, 0, Qt::AlignTop);
  root->addWidget(heroCard);

  QVBoxLayout* behaviorBody = nullptr;
  auto* behaviorCard = MakeCard(
    QStringLiteral("Behavior"),
    QStringLiteral("Core toggles stay near the top so the app’s operating mode is obvious at a glance."),
    this,
    &behaviorBody
  );
  auto* togglesLayout = new QHBoxLayout();
  togglesLayout->setSpacing(24);
  remapToggle_ = new QCheckBox(QStringLiteral("Enable remapping"), behaviorCard);
  autostartToggle_ = new QCheckBox(QStringLiteral("Start with Windows"), behaviorCard);
  togglesLayout->addWidget(remapToggle_);
  togglesLayout->addWidget(autostartToggle_);
  togglesLayout->addStretch(1);
  behaviorBody->addLayout(togglesLayout);
  root->addWidget(behaviorCard);

  QVBoxLayout* mappingsBody = nullptr;
  auto* mappingsCard = MakeCard(
    QStringLiteral("Button Mapping"),
    QStringLiteral("Each physical button can be rerouted to a different output without leaving the main window."),
    this,
    &mappingsBody
  );
  auto* mappingsGrid = new QGridLayout();
  mappingsGrid->setHorizontalSpacing(16);
  mappingsGrid->setVerticalSpacing(12);
  for (size_t i = 0; i < static_cast<size_t>(MouseButton::Count); ++i) {
    auto* label = new QLabel(QString::fromWCharArray(ButtonDisplayName(static_cast<MouseButton>(i))), mappingsCard);
    label->setObjectName(QStringLiteral("mapLabel"));
    auto* combo = new QComboBox(mappingsCard);
    for (size_t j = 0; j < static_cast<size_t>(MouseButton::Count); ++j) {
      combo->addItem(QString::fromWCharArray(ButtonDisplayName(static_cast<MouseButton>(j))));
    }
    mapCombos_[i] = combo;
    mappingsGrid->addWidget(label, static_cast<int>(i), 0);
    mappingsGrid->addWidget(combo, static_cast<int>(i), 1);

    QObject::connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, i](int index) {
      handleMappingChanged(i, index);
    });
  }
  mappingsBody->addLayout(mappingsGrid);
  root->addWidget(mappingsCard);

  QVBoxLayout* testerBody = nullptr;
  auto* testerCard = MakeCard(
    QStringLiteral("Input Tester"),
    QStringLiteral("Live mouse activity lights up here, including injected events so remap behavior is easy to verify."),
    this,
    &testerBody
  );
  auto* lightsGrid = new QGridLayout();
  lightsGrid->setHorizontalSpacing(10);
  lightsGrid->setVerticalSpacing(10);
  for (size_t i = 0; i < static_cast<size_t>(InputTesterLight::Count); ++i) {
    auto* light = new QLabel(QString::fromWCharArray(InputTesterLightName(static_cast<InputTesterLight>(i))), testerCard);
    light->setAlignment(Qt::AlignCenter);
    light->setMinimumHeight(42);
    light->setStyleSheet(LightStyle(false));
    inputTesterLights_[i] = light;
    const int row = static_cast<int>(i / 4);
    const int column = static_cast<int>(i % 4);
    lightsGrid->addWidget(light, row, column);
  }
  testerBody->addLayout(lightsGrid);

  inputTesterLog_ = new QPlainTextEdit(testerCard);
  inputTesterLog_->setReadOnly(true);
  inputTesterLog_->setPlainText(QStringLiteral("Awaiting input..."));
  inputTesterLog_->setMinimumHeight(110);
  testerBody->addWidget(inputTesterLog_);
  root->addWidget(testerCard, 1);

  QObject::connect(remapToggle_, &QCheckBox::toggled, this, [this](bool checked) {
    handleRemapToggled(checked);
  });
  QObject::connect(autostartToggle_, &QCheckBox::toggled, this, [this](bool checked) {
    handleAutostartToggled(checked);
  });
}

void ConfigWindow::clearInputTesterLights() {
  for (auto* light : inputTesterLights_) {
    if (light) {
      light->setStyleSheet(LightStyle(false));
    }
  }
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

void ConfigWindow::handleRemapToggled(bool checked) {
  g_app.settings.remapEnabled = checked;
  SaveSettings();
  SyncUiFromSettings();
}

void ConfigWindow::setTesterLightState(InputTesterLight light) {
  clearInputTesterLights();

  if (inputTesterLights_[ToIndex(InputTesterLight::Any)]) {
    inputTesterLights_[ToIndex(InputTesterLight::Any)]->setStyleSheet(LightStyle(true));
  }

  if (light != InputTesterLight::Any && inputTesterLights_[ToIndex(light)]) {
    inputTesterLights_[ToIndex(light)]->setStyleSheet(LightStyle(true));
  }
}

void ConfigWindow::updateInputTesterText(const MouseInputEvent& event) {
  if (!inputTesterLog_) {
    return;
  }

  inputTesterLog_->setPlainText(FormatInputTesterText(event));
  auto* bar = inputTesterLog_->verticalScrollBar();
  if (bar) {
    bar->setValue(bar->maximum());
  }
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