#include "selfdrive/ui/qt/sidebar.h"

#include <QMouseEvent>

#include "selfdrive/ui/qt/qt_window.h"
#include "selfdrive/common/util.h"
#include "selfdrive/hardware/hw.h"
#include "selfdrive/ui/qt/util.h"

void Sidebar::drawMetric(QPainter &p, const QString &label, const QString &val, QColor c, int y) {
  const QRect rect = {30, y, 240, val.isEmpty() ? (label.contains("\n") ? 124 : 100) : 148};

  p.setPen(Qt::NoPen);
  p.setBrush(QBrush(c));
  p.setClipRect(rect.x() + 6, rect.y(), 18, rect.height(), Qt::ClipOperation::ReplaceClip);
  p.drawRoundedRect(QRect(rect.x() + 6, rect.y() + 6, 100, rect.height() - 12), 10, 10);
  p.setClipping(false);

  QPen pen = QPen(QColor(0xff, 0xff, 0xff, 0x55));
  pen.setWidth(2);
  p.setPen(pen);
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(rect, 20, 20);

  p.setPen(QColor(0xff, 0xff, 0xff));
  if (val.isEmpty()) {
    configFont(p, "Open Sans", 35, "Bold");
    const QRect r = QRect(rect.x() + 30, rect.y(), rect.width() - 40, rect.height());
    p.drawText(r, Qt::AlignCenter, label);
  } else {
    configFont(p, "Open Sans", 58, "Bold");
    p.drawText(rect.x() + 50, rect.y() + 71, val);
    configFont(p, "Open Sans", 35, "Regular");
    p.drawText(rect.x() + 50, rect.y() + 50 + 77, label);
  }
}

Sidebar::Sidebar(QWidget *parent) : QFrame(parent) {
  home_img = QImage("../assets/images/button_home.png").scaled(180, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  settings_img = QImage("../assets/images/button_settings.png").scaled(settings_btn.width(), settings_btn.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

  connect(this, &Sidebar::valueChanged, [=] { update(); });

  setFixedWidth(300);
  setMinimumHeight(vwp_h);
  setStyleSheet("background-color: rgb(57, 57, 57);");
}

void Sidebar::mousePressEvent(QMouseEvent *event) {
  if (settings_btn.contains(event->pos())) {
    emit openSettings();
  }
}

void Sidebar::updateState(const UIState &s) {
  auto &sm = *(s.sm);

  auto deviceState = sm["deviceState"].getDeviceState();
  setProperty("netType", network_type[deviceState.getNetworkType()]);
  int strength = (int)deviceState.getNetworkStrength();
  setProperty("netStrength", strength > 0 ? strength + 1 : 0);
  setProperty("wifiAddr", deviceState.getWifiIpAddress().cStr());

  bool online = net_type != network_type[cereal::DeviceState::NetworkType::NONE];
  setProperty("connectStr",  online ? "ONLINE" : "OFFLINE");
  setProperty("connectStatus", online ? good_color : danger_color);

  QColor tempStatus = danger_color;
  auto ts = deviceState.getThermalStatus();
  if (ts == cereal::DeviceState::ThermalStatus::GREEN) {
    tempStatus = good_color;
  } else if (ts == cereal::DeviceState::ThermalStatus::YELLOW) {
    tempStatus = warning_color;
  }
  setProperty("tempStatus", tempStatus);
  setProperty("tempVal", (int)deviceState.getAmbientTempC());
  setProperty("BattPercent", (int)deviceState.getBatteryPercent());
  setProperty("BattStatus", deviceState.getBatteryStatus() == "Charging" ? 1 : 0);

  QString pandaStr = "VEHICLE\nONLINE";
  QColor pandaStatus = good_color;
  if (s.scene.pandaType == cereal::PandaState::PandaType::UNKNOWN) {
    pandaStatus = danger_color;
    pandaStr = "NO\nPANDA";
  } else if (s.scene.started && !sm["liveLocationKalman"].getLiveLocationKalman().getGpsOK()) {
    pandaStatus = warning_color;
    pandaStr = "GPS\nSEARCHING";
  }
  setProperty("pandaStr", pandaStr);
  setProperty("pandaStatus", pandaStatus);
}

void Sidebar::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  p.setPen(Qt::NoPen);
  p.setRenderHint(QPainter::Antialiasing);

  // static imgs
  p.setOpacity(0.65);
  p.drawImage(settings_btn.x(), settings_btn.y(), settings_img);
  p.setOpacity(1.0);
  p.drawImage(60, 1080 - 180 - 40, home_img);

  // batt percent and wifi ip
  p.drawImage(68, 180, battery_imgs[m_battery_img]); // signal_imgs to battery_imgs
  configFont(p, "Open Sans", 32, "Bold");
  p.setPen(QColor(0x00, 0x00, 0x00));
  const QRect r = QRect(80, 193, 100, 50);
  char battery_str[5];
  snprintf(battery_str, sizeof(battery_str), "%d%%", batt_percent);
  p.drawText(r, Qt::AlignCenter, battery_str);

  configFont(p, "Open Sans", 30, "Bold");
  p.setPen(QColor(0xff, 0xff, 0xff));

  const QRect r2 = QRect(0, 267, event->rect().width(), 50);
  if(Hardware::EON() && net_type == network_type[cereal::DeviceState::NetworkType::WIFI])
    p.drawText(r2, Qt::AlignCenter, wifi_addr);
  else
    p.drawText(r, Qt::AlignCenter, net_type);

  // metrics
  configFont(p, "Open Sans", 35, "Regular");
  drawMetric(p, "TEMP", QString("%1°C").arg(temp_val), temp_status, 338);
  drawMetric(p, panda_str, "", panda_status, 518);
  drawMetric(p, "CONNECT\n" + connect_str, "", connect_status, 676);
}
