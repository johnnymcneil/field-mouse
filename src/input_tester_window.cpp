#include "field_mouse/input_tester_window.h"

#include <QCloseEvent>
#include <QFile>
#include <QLabel>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTimer>

#include <optional>

#include "ui_input_tester_window.h"

#include "field_mouse/app_constants.h"

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

std::optional<InputTesterLight> InputTesterLightFromMessage(UINT message, DWORD mouseData) {
  switch (message) {
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
    return std::nullopt;
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

} // namespace

InputTesterWindow::InputTesterWindow(QWidget* parent)
  : QWidget(parent) {
  setAttribute(Qt::WA_QuitOnClose, false);
  setWindowFlag(Qt::WindowMinimizeButtonHint, true);
  setWindowFlag(Qt::WindowCloseButtonHint, true);

  buildUi();

  clearTimer_ = new QTimer(this);
  clearTimer_->setSingleShot(true);
  QObject::connect(clearTimer_, &QTimer::timeout, this, [this]() {
    clearInputTesterLights();
  });
}

InputTesterWindow::~InputTesterWindow() = default;

void InputTesterWindow::showAndActivate() {
  showNormal();
  raise();
  activateWindow();
}

void InputTesterWindow::handleInputTesterEvent(const MouseInputEvent& event) {
  const auto light = InputTesterLightFromMessage(event.message, event.mouseData);
  if (!light.has_value()) {
    return;
  }

  setTesterLightState(*light);
  updateInputTesterText(event);
  clearTimer_->start(140);
}

void InputTesterWindow::closeEvent(QCloseEvent* event) {
  hide();
  event->ignore();
}

void InputTesterWindow::applyStyles() {
  QFile styleFile(QStringLiteral(":/styles/config_window.qss"));
  if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    setStyleSheet({});
    return;
  }

  setStyleSheet(QString::fromUtf8(styleFile.readAll()));
}

void InputTesterWindow::buildUi() {
  ui_ = std::make_unique<Ui::InputTesterWindow>();
  ui_->setupUi(this);

  bindGeneratedWidgets();
  assignSemanticObjectNames();
  initializeInputTesterLights();
  applyStyles();
}

void InputTesterWindow::bindGeneratedWidgets() {
  inputTesterLog_ = ui_->inputTesterLogEdit;
  inputTesterLights_ = {
    ui_->lightLeftLabel,
    ui_->lightRightLabel,
    ui_->lightMiddleLabel,
    ui_->lightWheelLabel,
    ui_->lightX1Label,
    ui_->lightX2Label,
  };
}

void InputTesterWindow::assignSemanticObjectNames() {
  ui_->testerCardFrame->setObjectName(QStringLiteral("card"));
  ui_->testerTitleLabel->setObjectName(QStringLiteral("cardTitle"));
  ui_->testerSubtitleLabel->setObjectName(QStringLiteral("cardSubtitle"));
}

void InputTesterWindow::initializeInputTesterLights() {
  for (auto* light : inputTesterLights_) {
    if (light) {
      light->setStyleSheet(LightStyle(false));
    }
  }
}

void InputTesterWindow::clearInputTesterLights() {
  for (auto* light : inputTesterLights_) {
    if (light) {
      light->setStyleSheet(LightStyle(false));
    }
  }
}

void InputTesterWindow::setTesterLightState(InputTesterLight light) {
  clearInputTesterLights();

  if (inputTesterLights_[ToIndex(light)]) {
    inputTesterLights_[ToIndex(light)]->setStyleSheet(LightStyle(true));
  }
}

void InputTesterWindow::updateInputTesterText(const MouseInputEvent& event) {
  if (!inputTesterLog_) {
    return;
  }

  inputTesterLog_->setPlainText(FormatInputTesterText(event));
  auto* bar = inputTesterLog_->verticalScrollBar();
  if (bar) {
    bar->setValue(bar->maximum());
  }
}

} // namespace FieldMouse
