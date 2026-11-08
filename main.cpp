#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QEasingCurve>
#include <QElapsedTimer>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFontMetrics>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHash>
#include <QIcon>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QMainWindow>
#include <QLinearGradient>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QPair>
#include <QPixmap>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QScrollArea>
#include <QSettings>
#include <QShortcut>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <QStyle>
#include <QTimer>
#include <QTransform>
#include <QVariantAnimation>
#include <QVBoxLayout>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <limits>
#include <memory>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <utility>

struct Gpu {
    QString id;
    QString shortName;
    QString vendor;
    QString fullName;
    QString slot;
    QString vendorId;
    QString deviceId;
    QString driver;
    QJsonObject info;
    QJsonObject stats;
    bool haveStats = false;
    bool busy = false;
    double maxCore = 0;
    double maxMem = 0;
    double hwCore = 0;
    double hwMem = 0;
    int clockTries = 0;
};

struct View {
    QString throttle = "N/A", voltage = "N/A";
    QString core = "N/A", mem = "N/A", usage = "N/A", vram = "N/A", power = "N/A", fan = "N/A";
    QString vramTip, powerTip, tempTip, brief;
    double tGpu = 0, tHot = 0;
    bool hasGpuT = false, hasHotT = false;
    double coreR = 0, memR = 0, usageR = 0, vramR = 0, powerR = 0, fanR = 0;
    double coreS = 0, memS = 0, usageS = 0, vramS = 0, powerS = 0, fanS = 0;
    QString coreE, memE, usageE, vramE, powerE, fanE;
};

// App logo, drawn in code: a 2x2 image blown up with hard pixel edges (no scaling blur).
// top left white, bottom left black, bottom right orange, top right transparent.
static QImage logoImage()
{
    QImage img(2, 2, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    img.setPixelColor(0, 0, QColor("#ffffff"));
    img.setPixelColor(0, 1, QColor("#000000"));
    img.setPixelColor(1, 1, QColor("#ff9500"));
    return img;
}

// Crisp pixmap of the logo at `size` logical pixels (nearest neighbour, HiDPI aware).
static QPixmap logoPixmap(int size)
{
    const qreal dpr = qApp->devicePixelRatio();
    const int px = qRound(size * dpr);
    QPixmap pm = QPixmap::fromImage(logoImage().scaled(px, px, Qt::IgnoreAspectRatio, Qt::FastTransformation));
    pm.setDevicePixelRatio(dpr);
    return pm;
}

static QIcon makeLogo()
{
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128, 256})
        icon.addPixmap(QPixmap::fromImage(
            logoImage().scaled(size, size, Qt::IgnoreAspectRatio, Qt::FastTransformation)));   // fast = nearest neighbour
    return icon;
}

static const char *kGithubUrl = "https://github.com/4Fixxx/GPUPanel";

// "Made by 4Fix" with the name in the app's orange.
static QString madeByHtml()
{
    return QStringLiteral("Made by <span style=\"color:#ff9500;\">4Fix</span>");
}

static const char *kStyle = R"(
QWidget { background:#000000; color:#ffffff; font-size:14px; }
QLabel { background:transparent; }
QScrollArea { border:none; }
QDialog { border:1px solid #2a2a2a; }
QFrame#line { background:#2a2a2a; border:none; }
QFrame#slot { background:transparent; border:none; border-left:2px solid transparent; border-bottom:1px solid #1c1c1c; }
QFrame#slot:hover { background:#0e0e0e; }
QFrame#slot[selected="true"] { background:#121212; border-left:2px solid #ffffff; }
QLabel#name { font-size:15px; font-weight:600; }
QLabel#sub { color:#808080; font-size:12px; }
QLabel#live { color:#b8b8b8; font-size:12px; }
QLabel#key { color:#808080; }
QLabel#hkey { color:#808080; font-size:13px; }
QLabel#hname { font-size:20px; font-weight:600; }
QLabel#status { color:#808080; font-size:12px; }
QFrame#card { background:#0b0b0b; border:1px solid #232323; }
QFrame#card QWidget { background:transparent; }
QLabel#ckey { color:#808080; font-size:12px; }
QLabel#cval { font-size:16px; font-weight:600; }
QLabel#cval[na="true"] { color:#4a4a4a; }
QLabel#cval[lvl="yellow"] { color:#ffd60a; }
QLabel#cval[lvl="orange"] { color:#ff9500; }
QLabel#cval[lvl="red"] { color:#ff3b30; }
QLabel#cval[lvl="dark"] { color:#c40000; }
QPushButton#info { background:transparent; color:#808080; border:1px solid #3a3a3a; border-radius:0;
                   min-width:22px; max-width:22px; min-height:22px; max-height:22px; font-size:12px; }
QPushButton#info:hover { color:#ffffff; border-color:#ffffff; }
QPushButton#seg { background:transparent; color:#808080; border:1px solid #2a2a2a; border-radius:0;
                  padding:4px 10px; font-size:12px; }
QPushButton#seg:hover { color:#ffffff; border-color:#ffffff; }
QPushButton#seg:checked { background:#ffffff; color:#000000; border-color:#ffffff; }
QPushButton#seg:disabled { color:#3a3a3a; border-color:#1c1c1c; }
QWidget#osd { background:transparent; }
QComboBox { background:#0b0b0b; color:#ffffff; border:1px solid #2a2a2a; border-radius:0;
            padding:4px 10px; font-size:13px; }
QComboBox:hover { border-color:#ffffff; }
QComboBox:disabled { color:#4a4a4a; border-color:#1c1c1c; }
QComboBox QAbstractItemView { background:#000000; color:#ffffff; border:1px solid #3a3a3a;
                              selection-background-color:#ffffff; selection-color:#000000; outline:0; }
QToolTip { background:#000000; color:#ffffff; border:1px solid #3a3a3a; }
)";

static QJsonValue at(const QJsonValue &v, const char *path)
{
    QJsonValue cur = v;
    const QStringList keys = QString::fromLatin1(path).split('.');
    for (const QString &k : keys) {
        if (!cur.isObject())
            return QJsonValue(QJsonValue::Undefined);
        cur = cur.toObject().value(k);
    }
    return cur;
}

static bool numAt(const QJsonValue &v, const char *path, double &out)
{
    const QJsonValue x = at(v, path);
    if (!x.isDouble())
        return false;
    out = x.toDouble();
    return true;
}

static double findMax(const QJsonValue &v, const QStringList &keys)
{
    double best = 0;
    if (v.isObject()) {
        const QJsonObject o = v.toObject();
        for (auto it = o.begin(); it != o.end(); ++it) {
            const QJsonValue x = it.value();
            const bool match = keys.contains(it.key(), Qt::CaseInsensitive);
            if (match && x.isDouble()) {
                best = qMax(best, x.toDouble());
            } else if (match && x.isArray()) {
                const QJsonArray arr = x.toArray();
                for (const QJsonValue &e : arr)
                    if (e.isDouble())
                        best = qMax(best, e.toDouble());
            } else {
                best = qMax(best, findMax(x, keys));
            }
        }
    } else if (v.isArray()) {
        const QJsonArray a = v.toArray();
        for (const QJsonValue &e : a)
            best = qMax(best, findMax(e, keys));
    }
    return best;
}

static double maxStateValue(const QJsonValue &v, const char *arrayKey)
{
    double best = 0;
    const QJsonArray a = at(v, arrayKey).toArray();
    for (const QJsonValue &e : a) {
        const QJsonValue x = e.toObject().value("value");
        if (x.isDouble() && x.toDouble() < 20000)
            best = qMax(best, x.toDouble());
    }
    return best;
}

static QString describeError(const QJsonValue &d)
{
    if (d.isString())
        return d.toString();
    if (d.isObject())
        return QString::fromUtf8(QJsonDocument(d.toObject()).toJson(QJsonDocument::Compact));
    if (d.isArray())
        return QString::fromUtf8(QJsonDocument(d.toArray()).toJson(QJsonDocument::Compact));
    return "request failed";
}

static QString toShortName(const QString &dev)
{
    static const QRegularExpression br("\\[([^\\]]+)\\]");
    const auto m = br.match(dev);
    QString n = m.hasMatch() ? m.captured(1) : dev;
    n.remove(QRegularExpression("^(NVIDIA|AMD|Intel)\\s+", QRegularExpression::CaseInsensitiveOption));
    n.remove(QRegularExpression("^(GeForce|Radeon)\\s+"));
    return n.trimmed();
}

static QString toShortVendor(const QString &v)
{
    if (v.contains("NVIDIA", Qt::CaseInsensitive)) return "NVIDIA";
    if (v.contains("AMD") || v.contains("ATI")) return "AMD";
    if (v.contains("Intel")) return "Intel";
    return v;
}

static QString vendorFromId(const QString &id)
{
    const QString v = id.toLower();
    if (v == "10de") return "NVIDIA";
    if (v == "1002" || v == "1022") return "AMD";
    if (v == "8086") return "Intel";
    return id.toUpper();
}

static QString displayName(const Gpu &g)
{
    if (!g.shortName.isEmpty())
        return g.shortName;
    return g.vendor.isEmpty() ? QString("GPU") : g.vendor + " GPU";
}

static void parseId(Gpu &g)
{
    const QStringList parts = g.id.split('-');
    const QStringList dev = parts.first().split(':');
    if (dev.size() == 2) {
        g.vendorId = dev[0].toLower();
        g.deviceId = dev[1].toLower();
    }
    g.slot = parts.size() >= 2 ? parts.last() : g.id;
}

static void setName(Gpu &g, const QString &name)
{
    g.fullName = name;
    g.shortName = toShortName(name);
}

static void applyInfo(Gpu &g, const QJsonObject &info)
{
    g.info = info;
    g.driver = info.value("driver").toString();

    const QJsonObject dev = at(info, "pci_info.device_pci_info").toObject();
    const QString ven = dev.value("vendor").toString();
    const QString mod = dev.value("model").toString();
    if (!ven.isEmpty())
        g.vendor = toShortVendor(ven);
    if (g.fullName.isEmpty() && !mod.isEmpty()) {
        g.fullName = ven.isEmpty() ? mod : ven + " " + mod;
        g.shortName = toShortName(mod);
    }
}

static QString slotShort(const Gpu &g)
{
    return g.slot.count(':') >= 2 ? g.slot.section(':', 1) : g.slot;
}

class LactClient : public QObject {
public:
    using Reply = std::function<void(bool ok, const QJsonValue &data, const QString &error)>;
    std::function<void(bool up, const QString &why)> onState;

    explicit LactClient(QObject *parent = nullptr) : QObject(parent)
    {
        m_watchdog.setSingleShot(true);
        m_watchdog.setInterval(6000);
        QObject::connect(&m_watchdog, &QTimer::timeout, this,
                         [this] { drop("lactd stopped responding"); });
    }

    ~LactClient() override
    {
        if (m_fd >= 0)
            ::close(m_fd);
    }

    bool isUp() const { return m_fd >= 0; }

    static QStringList socketPaths()
    {
        QStringList out;
        for (const char *var : {"LACTD_SOCKET", "LACT_DAEMON_SOCKET_PATH"}) {
            const QByteArray env = qgetenv(var);
            if (!env.isEmpty())
                out << QString::fromLocal8Bit(env);
        }
        out << QString("/run/lactd.sock");
        out << QString("/run/user/%1/lactd.sock").arg(unsigned(::getuid()));
        const QByteArray xdg = qgetenv("XDG_RUNTIME_DIR");
        if (!xdg.isEmpty())
            out << QString::fromLocal8Bit(xdg) + "/lactd.sock";
        out.removeDuplicates();
        return out;
    }

    int lastErrno() const { return m_lastErrno; }

    bool open()
    {
        if (m_fd >= 0)
            return true;

        m_lastErrno = 0;
        QString firstErr;
        bool anyExists = false;
        for (const QString &path : socketPaths()) {
            const QByteArray p = path.toLocal8Bit();
            if (::access(p.constData(), F_OK) != 0)
                continue;
            anyExists = true;
            int e = 0;
            const int fd = connectTo(p, e);
            if (fd >= 0) {
                adopt(fd);
                return true;
            }
            if (firstErr.isEmpty()) {
                firstErr = path + ": " + errText(e);
                m_lastErrno = e;
            }
        }
        notify(false, anyExists ? firstErr : QString("no lactd socket (") + socketPaths().join(", ") + ")");
        return false;
    }

    void request(const QString &command, const QJsonObject &args, Reply reply)
    {
        if (m_fd < 0) {
            reply(false, QJsonValue(), "not connected");
            return;
        }
        QJsonObject o;
        o.insert("command", command);
        if (!args.isEmpty())
            o.insert("args", args);
        const QByteArray line = QJsonDocument(o).toJson(QJsonDocument::Compact) + '\n';

        m_wait.append(std::move(reply));
        const char *ptr = line.constData();
        qint64 left = line.size();
        while (left > 0) {
            const ssize_t n = ::send(m_fd, ptr, size_t(left), MSG_NOSIGNAL);
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                drop(errText(errno));
                return;
            }
            ptr += n;
            left -= n;
        }
        m_watchdog.start();
    }

private:
    static QString errText(int e) { return QString::fromLocal8Bit(std::strerror(e)); }

    static int connectTo(const QByteArray &p, int &err)
    {
        sockaddr_un addr;
        std::memset(&addr, 0, sizeof addr);
        addr.sun_family = AF_UNIX;
        if (p.size() >= int(sizeof addr.sun_path)) {
            err = ENAMETOOLONG;
            return -1;
        }
        std::memcpy(addr.sun_path, p.constData(), size_t(p.size()));
        const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0) {
            err = errno;
            return -1;
        }
        if (::connect(fd, reinterpret_cast<const sockaddr *>(&addr), sizeof addr) != 0) {
            err = errno;
            ::close(fd);
            return -1;
        }
        ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
        return fd;
    }

    void adopt(int fd)
    {
        m_fd = fd;
        m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
        QObject::connect(m_notifier, &QSocketNotifier::activated, this, [this] { readReady(); });
        notify(true, QString());
    }

    void notify(bool up, const QString &why)
    {
        if (onState)
            onState(up, why);
    }

    void drop(const QString &why)
    {
        if (m_fd < 0)
            return;
        if (m_notifier) {
            m_notifier->setEnabled(false);
            m_notifier->deleteLater();
            m_notifier = nullptr;
        }
        ::close(m_fd);
        m_fd = -1;
        m_buf.clear();
        m_watchdog.stop();

        const QList<Reply> pending = std::exchange(m_wait, QList<Reply>());
        for (const Reply &cb : pending)
            cb(false, QJsonValue(), why);
        notify(false, why);
    }

    void readReady()
    {
        char buf[65536];
        bool eof = false;
        for (;;) {
            const ssize_t n = ::read(m_fd, buf, sizeof buf);
            if (n > 0) {
                m_buf.append(buf, int(n));
                continue;
            }
            if (n == 0) {
                eof = true;
                break;
            }
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            drop(errText(errno));
            return;
        }

        for (;;) {
            const int nl = m_buf.indexOf('\n');
            if (nl < 0)
                break;
            const QByteArray line = m_buf.left(nl);
            m_buf.remove(0, nl + 1);
            dispatch(line);
            if (m_fd < 0)
                return;
        }
        if (eof)
            drop("lactd closed the connection");
    }

    void dispatch(const QByteArray &line)
    {
        if (m_wait.isEmpty())
            return;
        const Reply cb = m_wait.takeFirst();
        if (m_wait.isEmpty())
            m_watchdog.stop();
        else
            m_watchdog.start();

        QJsonParseError pe;
        const QJsonDocument doc = QJsonDocument::fromJson(line, &pe);
        if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
            cb(false, QJsonValue(), "invalid reply from lactd");
            return;
        }
        const QJsonObject o = doc.object();
        if (o.value("status").toString() == "ok")
            cb(true, o.value("data"), QString());
        else
            cb(false, o.value("data"), describeError(o.value("data")));
    }

    int m_fd = -1;
    int m_lastErrno = 0;
    QSocketNotifier *m_notifier = nullptr;
    QByteArray m_buf;
    QList<Reply> m_wait;
    QTimer m_watchdog;
};

static QString fmtNum(double v, double max)
{
    if (max >= 50)
        return QString::number(qRound(v));
    QString t = QString::number(v, 'f', 1);
    if (t.endsWith(".0"))
        t.chop(2);
    return t;
}

static QString fmtGiB(double bytes)
{
    return QString("%1 GiB").arg(bytes / 1073741824.0, 0, 'f', 1);
}

static View makeView(const Gpu &g)
{
    View v;
    const QJsonValue s(g.stats);
    double x = 0, y = 0;

    const QJsonValue th = at(s, "throttle_info");
    if (th.isObject()) {
        const QJsonObject o = th.toObject();
        QStringList parts;
        for (auto it = o.begin(); it != o.end(); ++it) {
            QStringList rs;
            const QJsonArray arr = it.value().toArray();
            for (const QJsonValue &r : arr)
                rs << r.toString();
            parts << (rs.isEmpty() ? it.key() : it.key() + ": " + rs.join(", "));
        }
        v.throttle = parts.isEmpty() ? "No" : parts.join(" | ");
    } else if (th.isArray()) {
        QStringList rs;
        const QJsonArray arr = th.toArray();
        for (const QJsonValue &r : arr)
            rs << r.toString();
        v.throttle = rs.isEmpty() ? "No" : rs.join(", ");
    }

    const QJsonObject temps = at(s, "temps").toObject();
    QStringList tl;
    double tFirst = 0;
    bool gotFirst = false;
    for (auto it = temps.begin(); it != temps.end(); ++it) {
        const QJsonValue tv = it.value();
        double t = 0;
        bool ok = false;
        if (tv.isDouble()) {
            t = tv.toDouble();
            ok = true;
        } else if (tv.isObject()) {
            const QJsonObject o = tv.toObject();
            const QJsonValue a = o.value("current"), b = o.value("value");
            if (a.isDouble()) { t = a.toDouble(); ok = true; }
            else if (b.isDouble()) { t = b.toDouble(); ok = true; }
        }
        if (!ok)
            continue;
        tl << it.key() + ": " + QString("%1°C").arg(qRound(t));
        const QString k = it.key().toLower();
        if (k.contains("junction") || k.contains("hotspot") || k.contains("hot_spot") || k.contains("hot spot")) {
            if (!v.hasHotT) { v.tHot = t; v.hasHotT = true; }
        } else if (k == "gpu" || k == "edge" || k == "core") {
            if (!v.hasGpuT) { v.tGpu = t; v.hasGpuT = true; }
        } else if (!gotFirst && !k.contains("mem") && !k.contains("vram")) {
            tFirst = t;
            gotFirst = true;
        }
    }
    if (!v.hasGpuT && gotFirst) {
        v.tGpu = tFirst;
        v.hasGpuT = true;
    }
    v.tempTip = tl.join("\n");
    const QString tempBrief = v.hasGpuT ? QString("%1°C").arg(qRound(v.tGpu)) : QString();

    if (numAt(s, "voltage.gpu", x))
        v.voltage = QString("%1 V").arg(x / 1000.0, 0, 'f', 3);

    if (numAt(s, "clockspeed.gpu_clockspeed", x) || numAt(s, "clockspeed.current_gfxclk", x)) {
        v.core = QString("%1 MHz").arg(qRound(x));
        v.coreR = g.maxCore > 0 ? x / g.maxCore : 0;
        v.coreS = g.maxCore;
        v.coreE = g.maxCore > 0 ? fmtNum(x, g.maxCore) : QString();
    }
    if (numAt(s, "clockspeed.vram_clockspeed", x)) {
        v.mem = QString("%1 MHz").arg(qRound(x));
        v.memR = g.maxMem > 0 ? x / g.maxMem : 0;
        v.memS = g.maxMem;
        v.memE = g.maxMem > 0 ? fmtNum(x, g.maxMem) : QString();
    }

    QString usageBrief;
    if (numAt(s, "busy_percent", x)) {
        v.usage = QString("%1%").arg(qRound(x));
        v.usageR = x / 100.0;
        v.usageS = 100;
        v.usageE = QString::number(qRound(x));
        usageBrief = v.usage;
    }

    if (numAt(s, "vram.used", x)) {
        v.vram = fmtGiB(x);
        if (numAt(s, "vram.total", y) && y > 0) {
            v.vramR = x / y;
            v.vramS = y / 1073741824.0;
            v.vramE = fmtNum(x / 1073741824.0, v.vramS);
            v.vramTip = fmtGiB(x) + " / " + fmtGiB(y);
        }
    }

    QString powerBrief;
    if (numAt(s, "power.average", x) || numAt(s, "power.current", x)) {
        v.power = QString("%1 W").arg(x, 0, 'f', 1);
        powerBrief = v.power;
        if (numAt(s, "power.cap_current", y) || numAt(s, "power.cap_max", y)) {
            if (y > 0) {
                v.powerR = x / y;
                v.powerS = y;
                v.powerE = fmtNum(x, y);
                v.powerTip = QString("%1 / %2 W").arg(x, 0, 'f', 1).arg(y, 0, 'f', 0);
            }
        }
    }

    double rpm = 0, pwm = 0, pwmMax = 0, rpmMax = 0;
    const bool hr = numAt(s, "fan.speed_current", rpm);
    const bool hp = numAt(s, "fan.pwm_current", pwm);
    numAt(s, "fan.pwm_max", pwmMax);
    numAt(s, "fan.speed_max", rpmMax);
    if (pwmMax <= 0)
        pwmMax = 255;
    const double pct = hp ? pwm * 100.0 / pwmMax : 0;
    if (hr && hp)
        v.fan = QString("%1 RPM (%2%)").arg(qRound(rpm)).arg(qRound(pct));
    else if (hr)
        v.fan = QString("%1 RPM").arg(qRound(rpm));
    else if (hp)
        v.fan = QString("%1%").arg(qRound(pct));
    if (hp) {
        v.fanR = pct / 100.0;
        v.fanS = 100;
        v.fanE = QString::number(qRound(pct));
    } else if (hr && rpmMax > 0) {
        v.fanR = rpm / rpmMax;
        v.fanS = rpmMax;
        v.fanE = QString::number(qRound(rpm));
    }

    QStringList brief;
    if (!tempBrief.isEmpty()) brief << tempBrief;
    if (!usageBrief.isEmpty()) brief << usageBrief;
    if (!powerBrief.isEmpty()) brief << powerBrief;
    v.brief = brief.isEmpty() ? QString("-") : brief.join("  ·  ");
    return v;
}

static void repolish(QWidget *w)
{
    w->style()->unpolish(w);
    w->style()->polish(w);
}

static void setValue(QLabel *l, const QString &t)
{
    const bool na = t == "N/A";
    if (l->property("na").toBool() != na || !l->property("na").isValid()) {
        l->setProperty("na", na);
        repolish(l);
    }
    l->setText(t);
}

static constexpr double kSkew = 16.0;
static constexpr int kLabelH = 14;

static QPolygonF para(double xl, double xr, double yt, double yb, double skew)
{
    QPolygonF poly;
    poly << QPointF(xl, yt) << QPointF(xr + skew, yt) << QPointF(xr, yb) << QPointF(xl, yb);
    return poly;
}

static void drawTicks(QPainter &p, double bw, double yt, double yb, int count)
{
    p.setPen(QPen(QColor(0, 0, 0, 150), 1));
    p.setBrush(Qt::NoBrush);
    for (int k = 1; k < count; ++k) {
        const double x = bw * k / count;
        p.drawLine(QPointF(x + kSkew, yt), QPointF(x, yb));
    }
}

static void drawScale(QPainter &p, double bw, double yt, double maxV, double width)
{
    QFont f = p.font();
    f.setPixelSize(10);
    p.setFont(f);
    const QFontMetrics fm(f);
    for (int k = 0; k <= 10; ++k) {
        const double x = k == 0 ? 0.0 : bw * k / 10.0 + kSkew;
        p.setPen(QPen(QColor("#707070"), 1));
        p.drawLine(QPointF(x, yt), QPointF(x + 2.0, yt - 4.0));
        const QString t = fmtNum(maxV * k / 10.0, maxV);
        const double tw = fm.horizontalAdvance(t);
        const double tx = qBound(0.0, x - tw / 2.0, width - tw);
        p.setPen(QColor("#9a9a9a"));
        p.drawText(QPointF(tx, yt - 5.0), t);
    }
}

static void drawEnd(QPainter &p, const QString &text, double edgeMid, double yMid, double minX,
                    double maxX, const QColor &inside, const QColor &outside)
{
    QFont f = p.font();
    f.setPixelSize(12);
    f.setBold(true);
    p.setFont(f);
    const QFontMetrics fm(f);
    const double tw = fm.horizontalAdvance(text);
    const double ty = yMid + (fm.ascent() - fm.descent()) / 2.0;
    if (edgeMid - minX >= tw + 10) {
        p.setPen(inside);
        p.drawText(QPointF(edgeMid - 5 - tw, ty), text);
    } else {
        p.setPen(outside);
        p.drawText(QPointF(qMin(edgeMid + 6, maxX - tw), ty), text);
    }
}

class BarWidget : public QWidget {
public:
    static constexpr int kBarH = 20;

    explicit BarWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setFixedHeight(kLabelH + kBarH);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void setData(double r, const QString &endText, double scaleMax)
    {
        if (!(r > 0))
            r = 0;
        if (r > 1)
            r = 1;
        if (qFuzzyCompare(r + 1.0, m_ratio + 1.0) && endText == m_end && qFuzzyCompare(scaleMax + 1.0, m_max + 1.0))
            return;
        m_ratio = r;
        m_end = endText;
        m_max = scaleMax;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const double yt = kLabelH;
        const double yb = kLabelH + kBarH;
        const double bw = qMax(1.0, width() - kSkew);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#262626"));
        p.drawPolygon(para(0, bw, yt, yb, kSkew));
        if (m_ratio > 0) {
            p.setBrush(QColor("#ffffff"));
            p.drawPolygon(para(0, bw * m_ratio, yt, yb, kSkew));
        }
        drawTicks(p, bw, yt, yb, 20);

        p.setPen(QPen(QColor("#666666"), 1));
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(para(0.5, bw - 0.5, yt + 0.5, yb - 0.5, kSkew));

        if (m_max > 0)
            drawScale(p, bw, yt, m_max, width());
        if (!m_end.isEmpty())
            drawEnd(p, m_end, bw * m_ratio + kSkew / 2.0, (yt + yb) / 2.0, 0.0, width(),
                    QColor("#000000"), QColor("#ffffff"));
    }

private:
    double m_ratio = 0;
    double m_max = 0;
    QString m_end;
};

static void setLevel(QLabel *l, const char *lvl)
{
    const QString s = QString::fromLatin1(lvl);
    if (l->property("lvl").toString() == s)
        return;
    l->setProperty("lvl", s);
    repolish(l);
}

static const char *tempLevel(double t)
{
    if (t > 100) return "dark";
    if (t > 90) return "red";
    if (t >= 75) return "orange";
    if (t >= 65) return "yellow";
    return "";
}

class TempBar : public QWidget {
public:
    static constexpr int kOver = 30;
    static constexpr int kTrackH = 20;
    static constexpr int kPad = 4;

    explicit TempBar(QWidget *parent = nullptr) : QWidget(parent)
    {
        setFixedHeight(kLabelH + kTrackH + 2 * kPad);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void setTemp(bool valid, double t)
    {
        if (valid == m_valid && qAbs(t - m_t) < 0.01)
            return;
        m_valid = valid;
        m_t = t;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const double hh = height();
        const double trackW = qMax(1, width() - kOver);
        const double ty = kLabelH + kPad;
        const double yb = ty + kTrackH;
        const double bw = qMax(1.0, trackW - kSkew);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#262626"));
        p.drawPolygon(para(0, bw, ty, yb, kSkew));

        p.setPen(QPen(QColor("#5e5e5e"), 1));
        for (int mark : {30, 65, 75, 90}) {
            const double x = bw * mark / 100.0;
            p.drawLine(QPointF(x + kSkew, ty), QPointF(x, yb));
        }

        const double t = m_t;
        const double r = qBound(0.0, t, 100.0) / 100.0;
        p.setPen(Qt::NoPen);
        if (m_valid && t > 100) {
            p.setBrush(QColor("#a80000"));
            p.drawPolygon(para(0, bw, ty, yb, kSkew));
        } else if (m_valid && r > 0) {
            const double fx = bw * r;
            if (t < 30) {
                QLinearGradient g(0, 0, fx + kSkew, 0);
                g.setColorAt(0, QColor("#12a0ff"));
                g.setColorAt(1, QColor("#ffffff"));
                p.setBrush(g);
            } else {
                QColor c("#ffffff");
                if (t > 90) c = QColor("#ff2d20");
                else if (t >= 75) c = QColor("#ff9500");
                else if (t >= 65) c = QColor("#ffd60a");
                p.setBrush(c);
            }
            p.drawPolygon(para(0, fx, ty, yb, kSkew));
        }
        drawTicks(p, bw, ty, yb, 10);

        p.setPen(QPen(QColor("#666666"), 1));
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(para(0.5, bw - 0.5, ty + 0.5, yb - 0.5, kSkew));

        drawScale(p, bw, ty, 100, width());

        if (m_valid && t > 100) {
            const double slope = kSkew / kTrackH;
            const double yTop = kLabelH;
            const double yBot = hh;
            const double xT = trackW - slope * (yTop - ty);
            const double xB = trackW - slope * (yBot - ty);
            const double extra = qBound(7.0, 7.0 + (t - 100.0) * 1.2, double(kOver - 5));
            QPolygonF over;
            over << QPointF(xT, yTop) << QPointF(xT + extra, yTop) << QPointF(xB + extra, yBot) << QPointF(xB, yBot);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#a80000"));
            p.drawPolygon(over);
        }

        if (m_valid) {
            const double edge = (t > 100 ? bw : bw * r) + kSkew / 2.0;
            drawEnd(p, QString::number(qRound(t)), edge, (ty + yb) / 2.0, 0.0, width(),
                    t > 100 ? QColor("#ffffff") : QColor("#000000"), QColor("#ffffff"));
        }
    }

private:
    bool m_valid = false;
    double m_t = 0;
};

class RowFrame : public QFrame {
public:
    std::function<void()> onClick;

protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton && onClick)
            onClick();
        QFrame::mousePressEvent(e);
    }
};

static QString prettyKey(const QString &k)
{
    static const QRegularExpression snake("^[a-z0-9_]+$");
    if (!snake.match(k).hasMatch())
        return k;
    static const QHash<QString, QString> special = {
        {"id", "ID"},         {"pci", "PCI"},       {"pcie", "PCIe"},     {"gpu", "GPU"},
        {"vram", "VRAM"},     {"drm", "DRM"},       {"opencl", "OpenCL"}, {"vulkan", "Vulkan"},
        {"vbios", "VBIOS"},   {"api", "API"},       {"uuid", "UUID"},     {"cpu", "CPU"},
        {"cuda", "CUDA"},     {"nvidia", "NVIDIA"}, {"amd", "AMD"},       {"mhz", "MHz"},
        {"sm", "SM"},         {"ecc", "ECC"},       {"mb", "MB"},         {"gb", "GB"}};
    QStringList w = k.split('_', Qt::SkipEmptyParts);
    for (QString &s : w)
        s = special.value(s, s.left(1).toUpper() + s.mid(1));
    QString out = w.join(' ');
    if (out.endsWith(" Info"))
        out.chop(5);
    return out;
}

static QString scalarText(const QJsonValue &v, const QString &key)
{
    if (v.isBool())
        return v.toBool() ? "Yes" : "No";
    if (v.isString()) {
        const QString s = v.toString();
        return s.isEmpty() ? QString("-") : s;
    }
    if (v.isDouble()) {
        const double d = v.toDouble();
        const QString k = key.toLower();
        if ((k.contains("memory") || k.contains("vram")) && d >= 1048576.0)
            return QString("%1 GiB").arg(d / 1073741824.0, 0, 'f', 2);
        return QString::number(d, 'g', 15);
    }
    return "-";
}

using Pairs = QList<QPair<QString, QString>>;

static void flatten(const QJsonValue &v, const QStringList &path, Pairs &out)
{
    const QString label = path.join(" › ");
    const QString last = path.isEmpty() ? QString() : path.last();

    if (v.isObject()) {
        const QJsonObject o = v.toObject();
        if (o.size() > 24) {
            out << qMakePair(label, QString("%1 entries").arg(o.size()));
            return;
        }
        for (auto it = o.begin(); it != o.end(); ++it)
            flatten(it.value(), path + QStringList{prettyKey(it.key())}, out);
        return;
    }
    if (v.isArray()) {
        const QJsonArray a = v.toArray();
        bool simple = true;
        for (const QJsonValue &e : a)
            if (e.isObject() || e.isArray())
                simple = false;
        if (simple) {
            if (a.size() > 24) {
                out << qMakePair(label, QString("%1 entries").arg(a.size()));
            } else {
                QStringList t;
                for (const QJsonValue &e : a)
                    t << scalarText(e, last);
                out << qMakePair(label, t.isEmpty() ? QString("-") : t.join(", "));
            }
        } else {
            for (int i = 0; i < a.size(); ++i)
                flatten(a.at(i), path + QStringList{QString::number(i + 1)}, out);
        }
        return;
    }
    out << qMakePair(label, scalarText(v, last));
}

static void showInfo(QWidget *parent, const Gpu g)
{
    QDialog d(parent);
    d.setWindowTitle(displayName(g));
    d.resize(680, 620);

    auto *outer = new QVBoxLayout(&d);
    outer->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *bodyW = new QWidget;
    auto *form = new QFormLayout(bodyW);
    form->setContentsMargins(24, 20, 24, 20);
    form->setHorizontalSpacing(24);
    form->setVerticalSpacing(10);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignTop);

    auto add = [&](const QString &k, const QString &v) {
        auto *kl = new QLabel(k);
        kl->setObjectName("key");
        kl->setWordWrap(true);
        auto *vl = new QLabel(v.isEmpty() ? "-" : v);
        vl->setWordWrap(true);
        vl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        form->addRow(kl, vl);
    };
    add("Name", g.fullName.isEmpty() ? displayName(g) : g.fullName);
    add("ID", g.id);
    add("Vendor", g.vendor);
    add("PCI slot", g.slot);
    add("Driver", g.driver);

    Pairs extra;
    for (auto it = g.info.begin(); it != g.info.end(); ++it) {
        if (it.key() == "driver")
            continue;
        flatten(it.value(), QStringList{prettyKey(it.key())}, extra);
    }
    for (const auto &p : extra)
        add(p.first, p.second);

    scroll->setWidget(bodyW);
    outer->addWidget(scroll);
    d.exec();
}

static QFrame *makeLine(bool vertical)
{
    auto *l = new QFrame;
    l->setObjectName("line");
    if (vertical) l->setFixedWidth(1);
    else l->setFixedHeight(1);
    return l;
}

// ---------------------------------------------------------------------------
// History graphs
// ---------------------------------------------------------------------------

enum ChartGroup { CgTemp, CgClock, CgLoad, CgVram, CgPower, CgVolt, CgFan, CgCount };

static const char *const kGroupTitle[CgCount] = {
    "Temperatures", "Clocks", "Load", "VRAM", "Power", "Voltage", "Fan speed",
};

static const char *const kPalette[] = {"#ffffff", "#12a0ff", "#ff9500", "#30d158",
                                       "#bf5af2", "#ffd60a", "#ff375f", "#64d2ff"};
static constexpr int kPaletteN = 8;

struct Reading {
    QString id;
    QString name;    // short, used in legends
    QString title;   // used when the reading has its own chart
    int group;
    double v;
    double scale;   // top of the matching bar (0 = that reading has no bar)
};

static QString tempName(const QString &key)
{
    const QString k = key.toLower();
    if (k.contains("junction") || k.contains("hotspot") || k.contains("hot_spot"))
        return "Hotspot";
    if (k == "mem" || k == "memory" || k == "vram")
        return "Memory";
    return prettyKey(key);
}

// Everything worth graphing in one stats snapshot. `scale` is the max of the
// matching bar, taken from the same View the bars are drawn from.
static QList<Reading> readingsOf(const Gpu &g)
{
    QList<Reading> out;
    const QJsonValue s(g.stats);
    const View bar = makeView(g);
    double x = 0;

    const QJsonObject temps = at(s, "temps").toObject();
    for (auto it = temps.begin(); it != temps.end(); ++it) {
        const QJsonValue tv = it.value();
        double t = 0;
        bool ok = false;
        if (tv.isDouble()) {
            t = tv.toDouble();
            ok = true;
        } else if (tv.isObject()) {
            const QJsonObject o = tv.toObject();
            const QJsonValue a = o.value("current"), b = o.value("value");
            if (a.isDouble()) { t = a.toDouble(); ok = true; }
            else if (b.isDouble()) { t = b.toDouble(); ok = true; }
        }
        if (ok) {
            const QString n = tempName(it.key());
            out.append(Reading{"t:" + it.key(), n, n + " temperature", CgTemp, t, 100.0});   // TempBar is 0..100
        }
    }

    if (numAt(s, "clockspeed.gpu_clockspeed", x) || numAt(s, "clockspeed.current_gfxclk", x))
        out.append(Reading{"clk:core", "Core", "GPU core clock", CgClock, x, bar.coreS});
    if (numAt(s, "clockspeed.vram_clockspeed", x))
        out.append(Reading{"clk:mem", "VRAM", "VRAM clock", CgClock, x, bar.memS});

    if (numAt(s, "busy_percent", x))
        out.append(Reading{"use:gpu", "GPU", "GPU usage", CgLoad, x, bar.usageS});
    if (numAt(s, "vram.used", x))
        out.append(Reading{"use:vram", "VRAM", "VRAM usage", CgVram, x / 1073741824.0, bar.vramS});

    double rpm = 0, pwm = 0, pwmMax = 0, rpmMax = 0;
    const bool hr = numAt(s, "fan.speed_current", rpm);
    const bool hp = numAt(s, "fan.pwm_current", pwm);
    numAt(s, "fan.pwm_max", pwmMax);
    numAt(s, "fan.speed_max", rpmMax);
    if (pwmMax <= 0)
        pwmMax = 255;
    if (hp)
        out.append(Reading{"use:fan", "Fan", "Fan speed (%)", CgLoad, pwm * 100.0 / pwmMax, 100.0});
    if (hr)
        out.append(Reading{"fan:rpm", "Fan", "Fan speed (RPM)", CgFan, rpm, rpmMax});

    if (numAt(s, "power.average", x) || numAt(s, "power.current", x))
        out.append(Reading{"pwr", "Power", "Power usage", CgPower, x, bar.powerS});
    if (numAt(s, "voltage.gpu", x))
        out.append(Reading{"volt", "GPU", "GPU voltage", CgVolt, x / 1000.0, 0.0});   // no bar for voltage
    return out;
}

struct SeriesData {
    QString id;
    QString name;
    QString title;
    int group = 0;
    QColor color;
    double scale = 0;    // latest bar max for this reading (0 = none)
    double peak = 0;     // highest value seen since app start
    QVector<double> v;   // same length as History::t, NaN = no value
};

// Samples of one GPU since app start. When it grows past kMax samples the
// oldest resolution is halved (every 2nd sample kept, sampling slows to match),
// so memory stays bounded while the whole run stays on the graph.
struct History {
    static constexpr int kMax = 20000;

    QVector<double> t;   // seconds since app start
    QList<SeriesData> series;
    int stride = 1;      // one stored sample per `stride` polls
    int skip = 0;

    bool hasGroup(int g) const
    {
        for (const SeriesData &s : series)
            if (s.group == g)
                return true;
        return false;
    }

    void add(double time, const QList<Reading> &rs)
    {
        if (++skip < stride)
            return;
        skip = 0;

        const double nan = std::numeric_limits<double>::quiet_NaN();
        QList<int> idx;
        for (const Reading &r : rs) {
            int k = -1;
            for (int i = 0; i < series.size(); ++i)
                if (series[i].id == r.id) { k = i; break; }
            if (k < 0) {
                SeriesData s;
                s.id = r.id;
                s.name = r.name;
                s.title = r.title;
                s.group = r.group;
                int same = 0;
                for (const SeriesData &o : series)
                    if (o.group == r.group)
                        ++same;
                s.color = QColor(kPalette[same % kPaletteN]);
                s.v.fill(nan, t.size());
                series.append(s);
                k = series.size() - 1;
            }
            idx << k;
        }

        t.append(time);
        for (SeriesData &s : series)
            s.v.append(nan);
        for (int i = 0; i < rs.size(); ++i) {
            SeriesData &s = series[idx[i]];
            s.v.last() = rs[i].v;
            s.scale = rs[i].scale;
            s.peak = qMax(s.peak, rs[i].v);
        }

        if (t.size() >= kMax)
            decimate();
    }

private:
    void decimate()
    {
        auto halve = [](QVector<double> &a) {
            QVector<double> o;
            o.reserve(a.size() / 2 + 1);
            for (int i = a.size() - 1; i >= 0; i -= 2)
                o.append(a[i]);
            std::reverse(o.begin(), o.end());
            a.swap(o);
        };
        halve(t);
        for (SeriesData &s : series)
            halve(s.v);
        stride *= 2;
        skip = 0;
    }
};

static QString fmtVal(int g, double v)
{
    switch (g) {
    case CgTemp:  return QString("%1°C").arg(qRound(v));
    case CgClock: return QString("%1 MHz").arg(qRound(v));
    case CgLoad:  return QString("%1%").arg(qRound(v));
    case CgVram:  return QString("%1 GiB").arg(v, 0, 'f', 1);
    case CgPower: return QString("%1 W").arg(v, 0, 'f', 1);
    case CgVolt:  return QString("%1 V").arg(v, 0, 'f', 3);
    default:      return QString("%1 RPM").arg(qRound(v));
    }
}

static QString fmtTime(double sec, bool hours)
{
    const int s = qMax(0, qRound(sec));
    const int h = s / 3600, m = (s % 3600) / 60, r = s % 60;
    if (hours)
        return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QLatin1Char('0')).arg(r, 2, 10, QLatin1Char('0'));
    return QString("%1:%2").arg(m + h * 60).arg(r, 2, 10, QLatin1Char('0'));
}

static double niceTimeStep(double span)
{
    static const double c[] = {1, 2, 5, 10, 15, 30, 60, 120, 300, 600, 900,
                               1800, 3600, 7200, 14400, 21600, 43200, 86400};
    for (double s : c)
        if (span / s <= 8)
            return s;
    return 86400;
}

// One chart: either every reading of a group (sid empty) or a single reading.
// The y axis always spans the real min..max of what is on screen.
class ChartWidget : public QWidget {
public:
    std::function<void(double)> onHover;   // time under the cursor, NaN = none

    ChartWidget(int group, const QString &sid, int height, QWidget *parent)
        : QWidget(parent), m_group(group), m_sid(sid)
    {
        setFixedHeight(height);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMouseTracking(true);
        hide();
    }

    void setHistory(std::shared_ptr<History> h)
    {
        m_h = std::move(h);
        m_ht = kNone;
        update();
    }

    void setWindow(double seconds)
    {
        m_window = seconds;
        update();
    }

    void setHoverTime(double t)
    {
        if (std::isnan(t) && std::isnan(m_ht))
            return;
        m_ht = t;
        update();
    }

    bool hasData() const
    {
        if (!m_h)
            return false;
        for (const SeriesData &s : m_h->series)
            if (uses(s))
                return true;
        return false;
    }

protected:
    void mouseMoveEvent(QMouseEvent *e) override
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const double px = e->position().x();
#else
        const double px = e->pos().x();
#endif
        double t = kNone;
        const QRectF plot = plotRect();
        if (m_h && !m_h->t.isEmpty() && px >= plot.left() && px <= plot.right()) {
            double x0 = 0, x1 = 1;
            timeRange(x0, x1);
            t = x0 + (px - plot.left()) / plot.width() * (x1 - x0);
        }
        setHoverTime(t);
        if (onHover)
            onHover(t);
    }

    void leaveEvent(QEvent *) override
    {
        setHoverTime(kNone);
        if (onHover)
            onHover(kNone);
    }

    void paintEvent(QPaintEvent *) override
    {
        if (!m_h || m_h->t.isEmpty())
            return;
        const QVector<double> &T = m_h->t;
        const QRectF plot = plotRect();
        if (plot.width() < 20 || plot.height() < 20)
            return;

        QFont lf = font();
        lf.setPixelSize(10);
        lf.setBold(false);
        QFont tf = font();
        tf.setPixelSize(12);
        tf.setBold(true);
        QFont vf = font();
        vf.setPixelSize(11);
        vf.setBold(false);
        const QFontMetrics lfm(lf);
        const QFontMetrics vfm(vf);

        double x0 = 0, x1 = 1;
        timeRange(x0, x1);
        int i0 = int(std::lower_bound(T.begin(), T.end(), x0) - T.begin());
        const int i1 = int(std::upper_bound(T.begin(), T.end(), x1) - T.begin());
        if (i0 > 0)
            --i0;

        // y axis = the bar's scale: 0 .. bar max (lines are never clipped: if a
        // reading goes above its bar max the top grows to fit it)
        double scale = 0, peak = 0, vmax = 0;
        QString title = kGroupTitle[m_group];
        bool titled = false;
        for (const SeriesData &s : m_h->series) {
            if (!uses(s))
                continue;
            if (!m_sid.isEmpty() && !titled) {
                title = s.title;
                titled = true;
            }
            scale = qMax(scale, s.scale);
            peak = qMax(peak, s.peak);
            for (int i = i0; i < i1; ++i)
                if (!std::isnan(s.v[i]))
                    vmax = qMax(vmax, s.v[i]);
        }
        double top = qMax(scale > 0 ? scale : peak, vmax);   // no bar -> highest value seen
        if (!(top > 0))
            top = 1;
        const double py0 = plot.top() + 6, py1 = plot.bottom() - 6;

        auto X = [&](double t) { return plot.left() + (t - x0) / (x1 - x0) * plot.width(); };
        auto Y = [&](double v) { return py1 - qMax(0.0, v) / top * (py1 - py0); };

        QPainter p(this);

        // plot area, grid, axis labels
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#050505"));
        p.drawRect(plot);

        p.setFont(lf);
        for (int k = 0; k <= 4; ++k) {
            const double v = top * k / 4.0;
            const double y = Y(v);
            p.setPen(QPen(QColor("#1c1c1c"), 1));
            p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
            const QString txt = m_group == CgVolt ? QString::number(v, 'f', 2) : fmtNum(v, top);
            p.setPen(QColor("#808080"));
            p.drawText(QPointF(plot.left() - 6 - lfm.horizontalAdvance(txt), y + 3.5), txt);
        }
        const double tStep = niceTimeStep(x1 - x0);
        const bool hours = x1 >= 3600;
        for (double tx = std::ceil(x0 / tStep) * tStep; tx <= x1 + 1e-9; tx += tStep) {
            if (tx < 0)
                continue;
            const double x = X(tx);
            p.setPen(QPen(QColor("#141414"), 1));
            p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
            const QString txt = fmtTime(tx, hours);
            const double w = lfm.horizontalAdvance(txt);
            p.setPen(QColor("#808080"));
            p.drawText(QPointF(qBound(0.0, x - w / 2.0, width() - w), plot.bottom() + 14), txt);
        }
        p.setPen(QPen(QColor("#2a2a2a"), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(plot);

        // lines (breaks at gaps, min/max envelope when there are more samples than pixels)
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setClipRect(plot);
        const double baseGap = qMax(5.0, 4.0 * m_h->stride);
        const int nVis = i1 - i0;
        const bool dense = nVis > int(plot.width() * 2);
        const int step = dense ? qMax(2, int(nVis / plot.width())) : 1;
        const double gap = dense ? qMax(baseGap, 3.0 * step * m_h->stride) : baseGap;
        for (const SeriesData &s : m_h->series) {
            if (!uses(s))
                continue;
            QPainterPath path;
            bool open = false;
            double lastT = 0;
            auto add = [&](int i) {
                const QPointF q(X(T[i]), Y(s.v[i]));
                if (!open || T[i] - lastT > gap) {
                    path.moveTo(q);
                    open = true;
                } else {
                    path.lineTo(q);
                }
                lastT = T[i];
            };
            for (int b = i0; b < i1; b += step) {
                const int e = qMin(b + step, i1);
                int mn = -1, mx = -1;
                for (int i = b; i < e; ++i) {
                    if (std::isnan(s.v[i]))
                        continue;
                    if (mn < 0 || s.v[i] < s.v[mn]) mn = i;
                    if (mx < 0 || s.v[i] > s.v[mx]) mx = i;
                }
                if (mn < 0) {
                    open = false;
                    continue;
                }
                add(qMin(mn, mx));
                if (mn != mx)
                    add(qMax(mn, mx));
            }
            p.setPen(QPen(s.color, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::NoBrush);
            p.drawPath(path);
        }
        p.setClipping(false);

        // hover: nearest sample to the hovered moment
        int hv = -1;
        if (!std::isnan(m_ht) && m_ht >= x0 && m_ht <= x1) {
            int k = int(std::lower_bound(T.begin(), T.end(), m_ht) - T.begin());
            if (k >= T.size())
                k = T.size() - 1;
            if (k > 0 && m_ht - T[k - 1] < T[k] - m_ht)
                --k;
            if (qAbs(T[k] - m_ht) <= baseGap)
                hv = k;
        }
        const bool single = !m_sid.isEmpty();

        // header: title + legend with the value at the hovered moment (or the latest one)
        p.setFont(tf);
        p.setPen(QColor("#b8b8b8"));
        p.drawText(QPointF(0, 14), title);
        double lx = QFontMetrics(tf).horizontalAdvance(title) + 20;
        const double limit = width() - (hv >= 0 ? 60 : 0);
        p.setFont(vf);
        for (const SeriesData &s : m_h->series) {
            if (!uses(s))
                continue;
            double v = hv >= 0 ? s.v[hv] : std::numeric_limits<double>::quiet_NaN();
            if (hv < 0) {
                for (int i = T.size() - 1; i >= 0 && i > T.size() - 60; --i)
                    if (!std::isnan(s.v[i])) { v = s.v[i]; break; }
            }
            const QString val = std::isnan(v) ? QString("N/A") : fmtVal(m_group, v);
            const double nw = single ? 0.0 : vfm.horizontalAdvance(s.name) + 6;
            const double w = 13 + nw + vfm.horizontalAdvance(val);
            if (lx + w > limit)
                break;
            p.setPen(Qt::NoPen);
            p.setBrush(s.color);
            p.drawRect(QRectF(lx, 6, 8, 8));
            if (!single) {
                p.setPen(QColor("#808080"));
                p.drawText(QPointF(lx + 13, 14), s.name);
            }
            p.setPen(QColor("#ffffff"));
            p.drawText(QPointF(lx + 13 + nw, 14), val);
            lx += w + 16;
        }

        if (hv < 0)
            return;

        // marker line + dots
        const double hx = X(T[hv]);
        p.setPen(QPen(QColor("#707070"), 1));
        p.drawLine(QPointF(hx, plot.top()), QPointF(hx, plot.bottom()));
        struct Row { QColor c; QString name, val; };
        QList<Row> rows;
        p.setPen(Qt::NoPen);
        for (const SeriesData &s : m_h->series) {
            if (!uses(s) || std::isnan(s.v[hv]))
                continue;
            p.setBrush(s.color);
            p.drawEllipse(QPointF(hx, Y(s.v[hv])), 3.0, 3.0);
            rows.append(Row{s.color, s.name, fmtVal(m_group, s.v[hv])});
        }
        const QString tt = fmtTime(T[hv], T.last() >= 3600);
        p.setPen(QColor("#808080"));
        p.drawText(QPointF(width() - 10 - vfm.horizontalAdvance(tt), 14), tt);

        // readout box next to the marker with the exact numbers at that moment
        if (rows.isEmpty())
            return;
        double bw = vfm.horizontalAdvance(tt);
        for (const Row &r : rows)
            bw = qMax(bw, 13.0 + (single ? 0.0 : vfm.horizontalAdvance(r.name) + 10) + vfm.horizontalAdvance(r.val));
        bw += 16;
        const double bh = 10 + 15 * (rows.size() + 1);
        double bx = hx + 10;
        if (bx + bw > plot.right())
            bx = hx - 10 - bw;
        bx = qMax(bx, plot.left() + 2);
        const double by = plot.top() + 4;
        p.setPen(QPen(QColor("#3a3a3a"), 1));
        p.setBrush(QColor(0, 0, 0, 235));
        p.drawRect(QRectF(bx, by, bw, bh));
        double ty = by + 15;
        p.setPen(QColor("#808080"));
        p.drawText(QPointF(bx + 8, ty), tt);
        for (const Row &r : rows) {
            ty += 15;
            p.setPen(Qt::NoPen);
            p.setBrush(r.c);
            p.drawRect(QRectF(bx + 8, ty - 8, 8, 8));
            double tx = bx + 21;
            if (!single) {
                p.setPen(QColor("#808080"));
                p.drawText(QPointF(tx, ty), r.name);
                tx += vfm.horizontalAdvance(r.name) + 10;
            }
            p.setPen(QColor("#ffffff"));
            p.drawText(QPointF(tx, ty), r.val);
        }
    }

private:
    static constexpr double kNone = std::numeric_limits<double>::quiet_NaN();

    bool uses(const SeriesData &s) const
    {
        return s.group == m_group && (m_sid.isEmpty() || s.id == m_sid);
    }

    QRectF plotRect() const
    {
        constexpr double kL = 46, kT = 28, kR = 10, kB = 20;
        return QRectF(kL, kT, width() - kL - kR, height() - kT - kB);
    }

    // whole run since app start, or the last m_window seconds
    void timeRange(double &x0, double &x1) const
    {
        x1 = m_h->t.last();
        x0 = m_window > 0 ? x1 - m_window : 0.0;
        if (x1 - x0 < 10)
            x1 = x0 + 10;
    }

    std::shared_ptr<History> m_h;
    int m_group;
    QString m_sid;
    double m_window = 0;
    double m_ht = kNone;
};

// Header with range buttons plus the charts. perSeries = one chart for every
// single reading, otherwise one chart per metric group. Hovering any chart
// moves the marker (and the readout) on all of them to the same moment.
class HistoryView : public QWidget {
public:
    std::function<void()> onFull;

    HistoryView(int chartH, int cols, double window, bool withFull, bool perSeries,
                QWidget *parent = nullptr)
        : QWidget(parent), m_cols(cols), m_chartH(chartH), m_per(perSeries), m_window(window)
    {
        auto *lay = new QVBoxLayout(this);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(10);

        auto *head = new QHBoxLayout;
        head->setSpacing(6);
        auto *title = new QLabel("History");
        title->setObjectName("ckey");
        head->addWidget(title);
        head->addStretch();

        auto *grp = new QButtonGroup(this);
        const QList<QPair<QString, double>> ranges = {{"1m", 60}, {"5m", 300}, {"15m", 900}, {"All", 0}};
        for (const auto &r : ranges) {
            auto *b = new QPushButton(r.first);
            b->setObjectName("seg");
            b->setCheckable(true);
            b->setCursor(Qt::PointingHandCursor);
            b->setChecked(qFuzzyCompare(r.second + 1.0, window + 1.0));
            grp->addButton(b);
            const double w = r.second;
            QObject::connect(b, &QPushButton::clicked, this, [this, w] { setWindow(w); });
            head->addWidget(b);
        }
        if (!withFull) {
            m_export = new QPushButton("Export PNG");
            m_export->setObjectName("seg");
            m_export->setCursor(Qt::PointingHandCursor);
            m_export->setToolTip("Save these graphs as a PNG image");
            m_export->setEnabled(false);
            QObject::connect(m_export, &QPushButton::clicked, this, [this] { exportPng(); });
            head->addSpacing(10);
            head->addWidget(m_export);
        }

        if (withFull) {
            auto *full = new QPushButton("Full graph");
            full->setObjectName("seg");
            full->setCursor(Qt::PointingHandCursor);
            full->setToolTip("A graph for every reading, since the app started");
            QObject::connect(full, &QPushButton::clicked, this, [this] {
                if (onFull)
                    onFull();
            });
            head->addSpacing(10);
            head->addWidget(full);
        }
        lay->addLayout(head);

        m_empty = new QLabel("No data yet");
        m_empty->setObjectName("sub");
        lay->addWidget(m_empty);

        m_grid = new QGridLayout;
        m_grid->setHorizontalSpacing(24);
        m_grid->setVerticalSpacing(14);
        for (int c = 0; c < cols; ++c)
            m_grid->setColumnStretch(c, 1);
        lay->addLayout(m_grid);
        lay->addStretch();
    }

    void setHistory(const std::shared_ptr<History> &h)
    {
        if (h == m_h)
            return;
        m_h = h;
        for (ChartWidget *c : m_byKey)
            c->setHistory(h);
        m_dirty = true;
    }

    // call after new samples arrived
    void refresh()
    {
        QStringList order;
        QList<QPair<int, QString>> specs;
        if (m_h) {
            for (int g = 0; g < CgCount; ++g) {
                if (m_per) {
                    for (const SeriesData &s : m_h->series)
                        if (s.group == g)
                            specs.append(qMakePair(g, s.id));
                } else if (m_h->hasGroup(g)) {
                    specs.append(qMakePair(g, QString()));
                }
            }
        }
        for (const auto &sp : specs) {
            const QString key = QString::number(sp.first) + "|" + sp.second;
            order << key;
            if (!m_byKey.contains(key)) {
                auto *c = new ChartWidget(sp.first, sp.second, m_chartH, this);
                c->setWindow(m_window);
                c->setHistory(m_h);
                c->onHover = [this](double t) { setHover(t); };
                m_byKey.insert(key, c);
            }
        }
        if (m_dirty || order != m_order) {
            m_dirty = false;
            m_order = order;
            relayout();
        }
        for (ChartWidget *c : m_byKey)
            c->update();
    }

    void setWindow(double seconds)
    {
        m_window = seconds;
        for (ChartWidget *c : m_byKey)
            c->setWindow(seconds);
    }

private:
    void setHover(double t)
    {
        for (ChartWidget *c : m_byKey)
            c->setHoverTime(t);
    }

    void relayout()
    {
        for (ChartWidget *c : m_byKey)
            m_grid->removeWidget(c);
        QList<ChartWidget *> shown;
        int n = 0;
        for (const QString &key : m_order) {
            ChartWidget *c = m_byKey.value(key);
            if (!c)
                continue;
            m_grid->addWidget(c, n / m_cols, n % m_cols);
            c->setVisible(true);
            shown << c;
            ++n;
        }
        for (ChartWidget *c : m_byKey)
            if (!shown.contains(c))
                c->setVisible(false);
        m_empty->setVisible(n == 0);
        if (m_export)
            m_export->setEnabled(n > 0);
    }

    QList<ChartWidget *> shownCharts() const
    {
        QList<ChartWidget *> out;
        for (const QString &key : m_order) {
            ChartWidget *c = m_byKey.value(key);
            if (c && !c->isHidden())
                out << c;
        }
        return out;
    }

    // Saves every graph in this window (not the header buttons) as one PNG.
    void exportPng()
    {
        const QList<ChartWidget *> shown = shownCharts();
        if (shown.isEmpty())
            return;

        const QString def =
            QDir(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation))
                .filePath("gpu-graph-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + ".png");
        QString path = QFileDialog::getSaveFileName(this, "Export graph as PNG", def, "PNG image (*.png)");
        if (path.isEmpty())
            return;
        if (!path.endsWith(".png", Qt::CaseInsensitive))
            path += ".png";

        QRect bounds;
        for (ChartWidget *c : shown)
            bounds = bounds.united(c->geometry());

        constexpr int kMargin = 16;
        const double scale = qMax(2.0, devicePixelRatioF());
        const int w = bounds.width() + 2 * kMargin;
        const int h = bounds.height() + 2 * kMargin;
        QImage img(int(std::lround(w * scale)), int(std::lround(h * scale)),
                   QImage::Format_ARGB32_Premultiplied);
        img.setDevicePixelRatio(scale);
        img.fill(QColor("#000000"));

        setHover(std::numeric_limits<double>::quiet_NaN());   // no hover marker in the export
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing, true);
        for (ChartWidget *c : shown)
            c->render(&p, c->geometry().topLeft() - bounds.topLeft() + QPoint(kMargin, kMargin));
        p.end();

        if (!img.save(path, "PNG"))
            QMessageBox::warning(this, "Export failed", "Could not write the image to:\n" + path);
    }

    int m_cols;
    int m_chartH;
    bool m_per;
    double m_window;
    bool m_dirty = true;
    QGridLayout *m_grid = nullptr;
    QLabel *m_empty = nullptr;
    QPushButton *m_export = nullptr;
    QHash<QString, ChartWidget *> m_byKey;
    QStringList m_order;
    std::shared_ptr<History> m_h;
};

struct Metric {
    QLabel *v = nullptr;
    BarWidget *b = nullptr;
};

struct TempMetric {
    QLabel *v = nullptr;
    TempBar *b = nullptr;
};

struct RowUi {
    RowFrame *frame = nullptr;
    QLabel *name = nullptr;
    QLabel *sub = nullptr;
    QLabel *live = nullptr;
};

// Square button with a triangle that spins between "open sidebar" (points right)
// and "close sidebar" (points left). progress: 0 = sidebar closed, 1 = open.
class SidebarToggle : public QAbstractButton {
public:
    explicit SidebarToggle(QWidget *parent = nullptr) : QAbstractButton(parent)
    {
        setFixedSize(32, 32);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);
    }

    void setProgress(double p)
    {
        m_p = std::clamp(p, 0.0, 1.0);
        update();
    }

    double progress() const { return m_p; }

    QSize sizeHint() const override { return QSize(32, 32); }

protected:
    bool event(QEvent *e) override
    {
        switch (e->type()) {
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
            update();
            break;
        default:
            break;
        }
        return QAbstractButton::event(e);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        const bool hot = underMouse() || isDown();

        p.setPen(QPen(QColor(hot ? "#ffffff" : "#2a2a2a"), 1));
        p.drawRect(rect().adjusted(0, 0, -1, -1));

        // Equilateral triangle centred on its centroid so it spins in place.
        constexpr double R = 7.0;
        const double h = R * std::sqrt(3.0) / 2.0;
        QPolygonF tri;
        tri << QPointF(R, 0) << QPointF(-R / 2.0, -h) << QPointF(-R / 2.0, h);

        p.setRenderHint(QPainter::Antialiasing, true);
        p.translate(width() / 2.0, height() / 2.0);
        p.rotate(180.0 * m_p);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(hot ? "#ffffff" : "#808080"));
        p.drawPolygon(tri);
    }

private:
    double m_p = 1.0;
};

// Holds a fixed-width child and reveals it by changing its own width. The child
// stays glued to the right edge, so shrinking the pane slides it out to the left
// instead of squashing its contents.
class SlidePane : public QWidget {
public:
    SlidePane(QWidget *content, int fullWidth) : m_content(content), m_full(fullWidth)
    {
        m_content->setParent(this);
        m_content->setVisible(true);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        setFixedWidth(fullWidth);
    }

    void setProgress(double p)
    {
        p = std::clamp(p, 0.0, 1.0);
        setFixedWidth(int(std::lround(m_full * p)));
    }

protected:
    void resizeEvent(QResizeEvent *) override
    {
        m_content->setGeometry(width() - m_full, 0, m_full, height());
    }

private:
    QWidget *m_content;
    int m_full;
};

// ---------------------------------------------------------------------------
// Overlay widgets: small always-on-top panels with live stats, drawn directly on
// the desktop (not inside the dashboard window). Several can exist at once
// (e.g. one per GPU); widgets that sit in the same corner stack instead of
// covering each other.
// ---------------------------------------------------------------------------

// Square menu button with three horizontal bars, same look as SidebarToggle.
class MenuButton : public QAbstractButton {
public:
    explicit MenuButton(QWidget *parent = nullptr) : QAbstractButton(parent)
    {
        setFixedSize(32, 32);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);
    }

    QSize sizeHint() const override { return QSize(32, 32); }

protected:
    bool event(QEvent *e) override
    {
        switch (e->type()) {
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
            update();
            break;
        default:
            break;
        }
        return QAbstractButton::event(e);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        const bool hot = underMouse() || isDown();

        p.setPen(QPen(QColor(hot ? "#ffffff" : "#2a2a2a"), 1));
        p.drawRect(rect().adjusted(0, 0, -1, -1));

        // three bars, 14 x 2 px, 5 px apart, centred in the button
        constexpr int barW = 14, barH = 2, pitch = 5;
        const int x = (width() - barW) / 2;
        const int y0 = (height() - (2 * pitch + barH)) / 2;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(hot ? "#ffffff" : "#808080"));
        for (int i = 0; i < 3; ++i)
            p.drawRect(x, y0 + i * pitch, barW, barH);
    }
};

// Square info button with a pixel-style "i", same look as MenuButton.
class InfoButton : public QAbstractButton {
public:
    explicit InfoButton(QWidget *parent = nullptr) : QAbstractButton(parent)
    {
        setFixedSize(32, 32);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);
    }

    QSize sizeHint() const override { return QSize(32, 32); }

protected:
    bool event(QEvent *e) override
    {
        switch (e->type()) {
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
            update();
            break;
        default:
            break;
        }
        return QAbstractButton::event(e);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        const bool hot = underMouse() || isDown();

        p.setPen(QPen(QColor(hot ? "#ffffff" : "#2a2a2a"), 1));
        p.drawRect(rect().adjusted(0, 0, -1, -1));

        // an "i": dot, stem, small serif on top and a base, centred in the button
        const int cx = width() / 2, y0 = (height() - 14) / 2;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(hot ? "#ffffff" : "#808080"));
        p.drawRect(cx - 1, y0, 2, 2);              // dot
        p.drawRect(cx - 1, y0 + 4, 2, 10);         // stem
        p.drawRect(cx - 3, y0 + 4, 2, 2);          // top serif
        p.drawRect(cx - 3, y0 + 12, 6, 2);         // base
    }
};

enum OsdCorner { OsdTopLeft, OsdTopRight, OsdBottomLeft, OsdBottomRight };

struct OsdCfg {
    bool enabled = false;          // off by default
    QString gpuId;                 // empty = first GPU
    int corner = OsdTopRight;      // top right by default
    bool sys = true;               // also show CPU load and RAM
};

struct OsdLine {
    QString label;
    QColor labelColor;
    QString value, unit;
    QString value2, unit2;         // optional second reading on the same line
};

class OsdWidget : public QWidget {
public:
    int corner = OsdTopRight;
    bool wanted = false;           // should be on screen (set from the config)

    // A top-level window of its own: no frame, no taskbar entry, never takes focus,
    // lets clicks fall through to whatever is underneath, and stays above other windows.
    OsdWidget() : QWidget(nullptr)
    {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                       Qt::WindowDoesNotAcceptFocus | Qt::WindowTransparentForInput |
                       Qt::NoDropShadowWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_QuitOnClose, false);   // closing the dashboard must still quit the app
        setObjectName("osd");      // transparent in the stylesheet, so the panel stays see-through
        setWindowTitle("gpu panel overlay");
        measure();
    }

    void setLines(const QList<OsdLine> &lines)
    {
        m_lines = lines;
        measure();
        update();
    }

    QSize sizeHint() const override { return m_size; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setPen(QColor(255, 255, 255, 38));
        p.setBrush(QColor(0, 0, 0, 210));
        p.drawRect(rect().adjusted(0, 0, -1, -1));

        const QFontMetrics vfm(m_vf), ufm(m_uf);
        const int x1 = kPadX + m_labelW + kLabelGap;                 // left edge of the 1st value column
        const int x2 = x1 + m_v1W + kUnitGap + m_u1W + kColGap;      // left edge of the 2nd value column
        const QColor orange("#ff9500");

        int y = kPadY;
        for (const OsdLine &l : m_lines) {
            const int base = y + vfm.ascent();
            const int top = y + ufm.ascent();                        // units sit at the top of the line
            const bool two = !l.value2.isEmpty();

            p.setFont(m_vf);
            p.setPen(l.labelColor);
            p.drawText(QPointF(kPadX, base), l.label);
            p.setPen(orange);
            p.drawText(QPointF(x1 + m_v1W - vfm.horizontalAdvance(l.value), base), l.value);
            if (two)
                p.drawText(QPointF(x2 + m_v2W - vfm.horizontalAdvance(l.value2), base), l.value2);

            p.setFont(m_uf);
            p.drawText(QPointF(x1 + m_v1W + kUnitGap, top), l.unit);
            if (two)
                p.drawText(QPointF(x2 + m_v2W + kUnitGap, top), l.unit2);
            y += m_rowH;
        }
    }

private:
    static constexpr int kPadX = 10, kPadY = 6, kLabelGap = 18, kUnitGap = 3, kColGap = 12;
    static constexpr int kValuePx = 15, kUnitPx = 10;

    // Column widths only ever grow with the text, never jitter: values are padded
    // to a minimum of 5 digits (RAM in MiB) and the second column to 3 (100 %).
    void measure()
    {
        m_vf = font();
        m_vf.setPixelSize(kValuePx);
        m_vf.setBold(true);
        m_uf = font();
        m_uf.setPixelSize(kUnitPx);
        m_uf.setBold(true);
        const QFontMetrics vfm(m_vf), ufm(m_uf);

        m_labelW = 0;
        m_v1W = vfm.horizontalAdvance("00000");
        m_v2W = 0;
        m_u1W = 0;
        m_u2W = 0;
        bool two = false;
        for (const OsdLine &l : m_lines) {
            m_labelW = qMax(m_labelW, vfm.horizontalAdvance(l.label));
            m_v1W = qMax(m_v1W, vfm.horizontalAdvance(l.value));
            m_u1W = qMax(m_u1W, ufm.horizontalAdvance(l.unit));
            if (!l.value2.isEmpty()) {
                two = true;
                m_v2W = qMax(m_v2W, vfm.horizontalAdvance(l.value2));
                m_u2W = qMax(m_u2W, ufm.horizontalAdvance(l.unit2));
            }
        }
        if (two)
            m_v2W = qMax(m_v2W, vfm.horizontalAdvance("000"));

        m_rowH = vfm.height() + 2;
        int w = kPadX + m_labelW + kLabelGap + m_v1W + kUnitGap + m_u1W + kPadX;
        if (two)
            w += kColGap + m_v2W + kUnitGap + m_u2W;
        m_size = QSize(w, 2 * kPadY + m_rowH * qMax(1, int(m_lines.size())));
    }

    QList<OsdLine> m_lines;
    QFont m_vf, m_uf;
    int m_labelW = 0, m_v1W = 0, m_v2W = 0, m_u1W = 0, m_u2W = 0, m_rowH = 0;
    QSize m_size;
};

// Places the overlay windows in the corners of the primary screen and stacks the
// ones that share a corner. Not a widget: the overlays are independent windows.
class OsdHost {
public:
    QList<OsdWidget *> osd;

    ~OsdHost() { qDeleteAll(osd); }

    // Positions every wanted overlay and shows / hides the windows to match.
    void relayout()
    {
        QScreen *screen = QGuiApplication::primaryScreen();
        constexpr int mx = 18, my = 14, gap = 8;
        const QRect area = screen ? screen->availableGeometry() : QRect();   // excludes panels / taskbars
        int used[4] = {my, my, my, my};            // space already taken in each corner
        for (OsdWidget *w : osd) {
            if (!w->wanted || !screen) {
                if (w->isVisible())
                    w->hide();
                continue;
            }
            const QSize s = w->sizeHint();
            const int c = qBound(0, w->corner, 3);
            const bool right = c == OsdTopRight || c == OsdBottomRight;
            const bool bottom = c == OsdBottomLeft || c == OsdBottomRight;
            const int x = right ? area.right() + 1 - mx - s.width() : area.left() + mx;
            const int y = bottom ? area.bottom() + 1 - used[c] - s.height() : area.top() + used[c];
            if (w->size() != s)
                w->setFixedSize(s);
            if (w->pos() != QPoint(x, y))
                w->move(x, y);
            if (!w->isVisible())
                w->show();
            used[c] += s.height() + gap;
        }
    }
};

class Panel {
public:
    Panel()
    {
        win.setWindowTitle("gpu panel");
        win.resize(1373, 864);
        uptime.start();

        lact = new LactClient(&win);
        lact->onState = [this](bool up, const QString &why) { onLactState(up, why); };

        auto *central = new QWidget;
        auto *body = new QHBoxLayout(central);
        body->setContentsMargins(0, 0, 0, 0);
        body->setSpacing(0);

        auto *main = new QWidget;
        auto *mainLay = new QVBoxLayout(main);
        mainLay->setContentsMargins(0, 0, 0, 0);
        mainLay->setSpacing(0);

        auto *toggle = new SidebarToggle;
        toggle->setToolTip("Toggle sidebar (Ctrl+B)");
        auto *top = new QHBoxLayout;
        top->setContentsMargins(8, 8, 8, 8);
        top->addWidget(toggle);

        auto *brandLogo = new QLabel;
        brandLogo->setPixmap(logoPixmap(32));
        brandLogo->setFixedSize(32, 32);
        auto *brandName = new QLabel("GPU Panel");
        brandName->setObjectName("name");
        auto *brandBy = new QLabel(madeByHtml());
        brandBy->setObjectName("sub");
        auto *brandText = new QVBoxLayout;
        brandText->setContentsMargins(0, 0, 0, 0);
        brandText->setSpacing(0);
        brandText->addWidget(brandName);
        brandText->addWidget(brandBy);
        auto *brandW = new QWidget;
        auto *brand = new QHBoxLayout(brandW);
        brand->setContentsMargins(0, 0, 0, 0);
        brand->setSpacing(10);
        brand->addWidget(brandLogo);
        brand->addLayout(brandText);
        brandW->ensurePolished();                          // so sizeHint() already has the stylesheet fonts
        const int brandGap = 8 + brandW->sizeHint().width() / 2;   // gap after the toggle: half the brand's width
        top->addSpacing(brandGap * 40 / 100);              // moved back left by 60 % of that free space
        top->addWidget(brandW);

        top->addStretch();
        auto *infoBtn = new InfoButton;
        infoBtn->setToolTip("About");
        QObject::connect(infoBtn, &QAbstractButton::clicked, &win, [this] { showAbout(); });
        top->addWidget(infoBtn);
        top->addSpacing(8);                                // a bit of air between the two buttons
        auto *menu = new MenuButton;
        menu->setToolTip("Overlay widgets");
        QObject::connect(menu, &QAbstractButton::clicked, &win, [this] { showOsdSettings(); });
        top->addWidget(menu);
        mainLay->addLayout(top);

        auto *hr = new QHBoxLayout;
        hr->setContentsMargins(16, 8, 16, 0);
        hr->addWidget(makeLine(false));
        mainLay->addLayout(hr);

        auto *nameRow = new QHBoxLayout;
        nameRow->setContentsMargins(16, 18, 16, 16);
        nameRow->setSpacing(12);
        auto *hkey = new QLabel("GPU");
        hkey->setObjectName("hkey");
        hname = new QLabel("-");
        hname->setObjectName("hname");
        status = new QLabel;
        status->setObjectName("status");
        status->hide();
        nameRow->addWidget(hkey);
        nameRow->addWidget(hname);
        nameRow->addStretch();
        nameRow->addWidget(status);
        mainLay->addLayout(nameRow);

        auto *cardsW = new QWidget;
        auto *cards = new QVBoxLayout(cardsW);
        cards->setContentsMargins(16, 0, 16, 16);
        cards->setSpacing(12);

        QFont mf = QApplication::font();
        mf.setPixelSize(16);
        mf.setWeight(QFont::Bold);
        const int valW = QFontMetrics(mf).horizontalAdvance(QString(16, QLatin1Char('0'))) + 4;

        auto *c1 = new QFrame;
        c1->setObjectName("card");
        auto *g1 = new QGridLayout(c1);
        g1->setContentsMargins(18, 16, 18, 18);
        g1->setHorizontalSpacing(36);
        g1->setVerticalSpacing(18);
        g1->setColumnStretch(0, 1);
        g1->setColumnStretch(1, 1);
        throttle = textCell(g1, "Throttling", 0, 0, 1);
        voltage = textCell(g1, "GPU Voltage", 0, 1, 1);
        tgpu = tempCell(g1, "GPU Temperature", 1, valW);
        thot = tempCell(g1, "GPU Hotspot Temperature", 2, valW);
        cards->addWidget(c1);

        auto *c2 = new QFrame;
        c2->setObjectName("card");
        auto *g2 = new QGridLayout(c2);
        g2->setContentsMargins(18, 16, 18, 18);
        g2->setHorizontalSpacing(36);
        g2->setVerticalSpacing(18);
        g2->setColumnStretch(0, 1);
        core = barCell(g2, "GPU Core Clock", 0, 0, valW);
        usage = barCell(g2, "GPU Usage", 1, 0, valW);
        power = barCell(g2, "Power Usage", 2, 0, valW);
        mem = barCell(g2, "VRAM Clock", 3, 0, valW);
        vram = barCell(g2, "VRAM Usage", 4, 0, valW);
        fan = barCell(g2, "Fan Speed", 5, 0, valW);
        cards->addWidget(c2);

        auto *c3 = new QFrame;
        c3->setObjectName("card");
        auto *l3 = new QVBoxLayout(c3);
        l3->setContentsMargins(18, 16, 18, 18);
        hv = new HistoryView(150, 2, 300, true, false);
        hv->onFull = [this] {
            if (!currentId.isEmpty())
                showFull(currentId);
        };
        l3->addWidget(hv);
        cards->addWidget(c3);
        cards->addStretch();

        auto *cardsScroll = new QScrollArea;
        cardsScroll->setWidgetResizable(true);
        cardsScroll->setFrameShape(QFrame::NoFrame);
        cardsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        cardsScroll->setWidget(cardsW);
        osdHost = new OsdHost;                 // overlay widgets live on the desktop, not in this window
        mainLay->addWidget(cardsScroll, 1);

        scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setFixedWidth(240);
        vline = makeLine(true);
        rebuildSidebar();

        sidebar = new SlidePane(scroll, 240);

        sidebarAnim = new QVariantAnimation(&win);
        sidebarAnim->setEasingCurve(QEasingCurve::InOutCubic);
        QObject::connect(sidebarAnim, &QVariantAnimation::valueChanged, &win,
                         [this, toggle](const QVariant &v) {
                             const double p = v.toDouble();
                             sidebar->setProgress(p);
                             toggle->setProgress(p);
                         });
        QObject::connect(sidebarAnim, &QVariantAnimation::finished, &win, [this] {
            if (!sidebarOpen) {
                sidebar->hide();
                vline->hide();
            }
        });

        auto toggleSidebar = [this, toggle] {
            sidebarOpen = !sidebarOpen;
            const double from = toggle->progress();
            const double to = sidebarOpen ? 1.0 : 0.0;
            if (sidebarOpen) {
                sidebar->show();
                vline->show();
            }
            sidebarAnim->stop();
            sidebarAnim->setStartValue(from);
            sidebarAnim->setEndValue(to);
            // Scale duration by remaining distance so mid-animation reversals feel natural.
            sidebarAnim->setDuration(std::max(80, int(300 * std::abs(to - from))));
            sidebarAnim->start();
        };
        QObject::connect(toggle, &QAbstractButton::clicked, &win, toggleSidebar);
        auto *sc = new QShortcut(QKeySequence("Ctrl+B"), &win);
        QObject::connect(sc, &QShortcut::activated, toggleSidebar);

        body->addWidget(sidebar);
        body->addWidget(vline);
        body->addWidget(main, 1);
        win.setCentralWidget(central);

        loadOsd();
        syncOsd();
        render();

        auto *timer = new QTimer(&win);
        QObject::connect(timer, &QTimer::timeout, &win, [this] { poll(); });
        timer->start(1000);
        QTimer::singleShot(0, &win, [this] { poll(); });
    }

    ~Panel()
    {
        delete osdHost;
        if (userDaemon && userDaemon->state() != QProcess::NotRunning) {
            userDaemon->terminate();
            if (!userDaemon->waitForFinished(1500))
                userDaemon->kill();
        }
    }

    void show() { win.show(); }

private:

    // -----------------------------------------------------------------
    // Overlay widgets
    // -----------------------------------------------------------------

    void loadOsd()
    {
        QSettings s("gpu-panel", "gpu-panel");
        osdCfg.clear();
        if (!s.contains("osd/widgets")) {
            osdCfg << OsdCfg();                    // first run: one widget, switched off
            return;
        }
        const QJsonArray a = QJsonDocument::fromJson(s.value("osd/widgets").toString().toUtf8()).array();
        for (const QJsonValue &v : a) {
            const QJsonObject o = v.toObject();
            OsdCfg c;
            c.enabled = o.value("enabled").toBool(false);
            c.gpuId = o.value("gpu").toString();
            c.corner = qBound(0, o.value("corner").toInt(OsdTopRight), 3);
            c.sys = o.value("sys").toBool(true);
            osdCfg << c;
        }
    }

    void saveOsd()
    {
        QJsonArray a;
        for (const OsdCfg &c : osdCfg)
            a.append(QJsonObject{{"enabled", c.enabled}, {"gpu", c.gpuId}, {"corner", c.corner}, {"sys", c.sys}});
        QSettings s("gpu-panel", "gpu-panel");
        s.setValue("osd/widgets", QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact)));
    }

    // Make the widgets on screen match osdCfg (create / delete / move / show / hide).
    void syncOsd()
    {
        if (!osdHost)
            return;
        while (osdHost->osd.size() > osdCfg.size()) {
            OsdWidget *w = osdHost->osd.takeLast();
            w->hide();
            w->deleteLater();
        }
        while (osdHost->osd.size() < osdCfg.size())
            osdHost->osd << new OsdWidget;
        for (int i = 0; i < osdCfg.size(); ++i) {
            osdHost->osd[i]->corner = osdCfg[i].corner;
            osdHost->osd[i]->wanted = osdCfg[i].enabled;   // updateOsd() below shows / hides / places it
        }
        if (osdWantsSys())
            sampleSystem();
        updateOsd();
    }

    void commitOsd()
    {
        saveOsd();
        syncOsd();
    }

    bool osdWantsSys() const
    {
        for (const OsdCfg &c : osdCfg)
            if (c.enabled && c.sys)
                return true;
        return false;
    }

    static QByteArray readProc(const char *path)
    {
        QFile f(QString::fromLatin1(path));
        return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
    }

    // CPU load (needs two samples, so the first value shows up one tick late) and RAM in use.
    void sampleSystem()
    {
        const QList<QByteArray> cpu = readProc("/proc/stat").split('\n').value(0).simplified().split(' ');
        if (cpu.size() >= 9 && cpu[0] == "cpu") {
            quint64 total = 0;
            for (int i = 1; i <= 8; ++i)             // user nice system idle iowait irq softirq steal
                total += cpu[i].toULongLong();
            const quint64 idle = cpu[4].toULongLong() + cpu[5].toULongLong();
            if (cpuPrevTotal && total > cpuPrevTotal) {
                const double dt = double(total - cpuPrevTotal);
                const double di = double(idle - cpuPrevIdle);
                sysCpu = qBound(0.0, 100.0 * (dt - di) / dt, 100.0);
            }
            cpuPrevTotal = total;
            cpuPrevIdle = idle;
        }

        qint64 total = -1, avail = -1;
        const QList<QByteArray> mem = readProc("/proc/meminfo").split('\n');
        for (const QByteArray &line : mem) {
            if (line.startsWith("MemTotal:"))
                total = line.mid(9).simplified().split(' ').value(0).toLongLong();
            else if (line.startsWith("MemAvailable:"))
                avail = line.mid(13).simplified().split(' ').value(0).toLongLong();
        }
        if (total >= 0 && avail >= 0)
            sysRamMiB = (total - avail) / 1024.0;
    }

    QList<OsdLine> osdLines(const OsdCfg &c)
    {
        static const QColor green("#30d158"), blue("#12a0ff");
        const QString dash = "--";

        const Gpu *g = c.gpuId.isEmpty() ? nullptr : find(c.gpuId);
        if (!g && !gpus.isEmpty())
            g = &gpus.first();                      // unset or vanished GPU: fall back to the first one
        const bool live = g && g->haveStats;
        const QJsonValue s = live ? QJsonValue(g->stats) : QJsonValue();

        QString temp = dash, usage = dash, clock = dash, power = dash, fan = dash, fanUnit = "%";
        double x = 0;
        if (live) {
            const View v = makeView(*g);
            if (v.hasGpuT)
                temp = QString::number(qRound(v.tGpu));
            if (numAt(s, "busy_percent", x))
                usage = QString::number(qRound(x));
            if (numAt(s, "clockspeed.gpu_clockspeed", x) || numAt(s, "clockspeed.current_gfxclk", x))
                clock = QString::number(qRound(x));
            if (numAt(s, "power.average", x) || numAt(s, "power.current", x))
                power = QString::number(qRound(x));

            double rpm = 0, pwm = 0, pwmMax = 0;
            const bool hasRpm = numAt(s, "fan.speed_current", rpm);
            const bool hasPwm = numAt(s, "fan.pwm_current", pwm);
            numAt(s, "fan.pwm_max", pwmMax);
            if (pwmMax <= 0)
                pwmMax = 255;
            if (hasPwm) {
                fan = QString::number(qRound(pwm * 100.0 / pwmMax));
            } else if (hasRpm) {
                fan = QString::number(qRound(rpm));
                fanUnit = "RPM";
            }
        }

        QList<OsdLine> out;
        out.append({g ? displayName(*g) : QString("GPU"), green, temp, "°C", usage, "%"});
        out.append({"GPU Clock", green, clock, "MHz", power, "W"});
        out.append({"GPU Fan Speed", green, fan, fanUnit, QString(), QString()});
        if (c.sys) {
            out.append({"CPU", blue, sysCpu >= 0 ? QString::number(qRound(sysCpu)) : dash, "%",
                        QString(), QString()});
            out.append({"RAM", blue, sysRamMiB >= 0 ? QString::number(qRound(sysRamMiB)) : dash, "MiB",
                        QString(), QString()});
        }
        return out;
    }

    void updateOsd()
    {
        if (!osdHost)
            return;
        for (int i = 0; i < osdCfg.size() && i < osdHost->osd.size(); ++i)
            if (osdCfg[i].enabled)
                osdHost->osd[i]->setLines(osdLines(osdCfg[i]));
        osdHost->relayout();
    }

    void addOsdWidget()
    {
        auto effective = [this](const OsdCfg &o) {
            return o.gpuId.isEmpty() && !gpus.isEmpty() ? gpus.first().id : o.gpuId;
        };
        OsdCfg c;
        c.enabled = true;                           // you just asked for it, so show it
        for (const Gpu &g : gpus) {                 // prefer a GPU that has no widget yet
            bool used = false;
            for (const OsdCfg &o : osdCfg)
                used = used || effective(o) == g.id;
            if (!used) {
                c.gpuId = g.id;
                break;
            }
        }
        c.corner = osdCfg.isEmpty() ? int(OsdTopRight) : osdCfg.last().corner;   // same corner = stacks
        c.sys = osdCfg.isEmpty();                   // CPU/RAM once is enough
        osdCfg << c;
        commitOsd();
    }

    void showAbout()
    {
        QDialog d(&win);
        d.setWindowTitle("GPU Panel");

        auto *lay = new QVBoxLayout(&d);
        lay->setContentsMargins(48, 32, 48, 24);
        lay->setSpacing(6);
        lay->setSizeConstraint(QLayout::SetFixedSize);

        auto *logo = new QLabel;
        logo->setPixmap(logoPixmap(64));
        logo->setAlignment(Qt::AlignCenter);
        auto *name = new QLabel("GPU Panel");
        name->setObjectName("hname");
        name->setAlignment(Qt::AlignCenter);
        auto *by = new QLabel(madeByHtml());
        by->setObjectName("sub");
        by->setAlignment(Qt::AlignCenter);

        auto *link = new QLabel(QStringLiteral("<a href=\"%1\"><span style=\"color:#ffffff; text-decoration:underline;\">"
                                               "github.com/4Fixxx/GPUPanel</span></a>").arg(QString::fromLatin1(kGithubUrl)));
        link->setObjectName("sub");
        link->setAlignment(Qt::AlignCenter);
        link->setOpenExternalLinks(true);
        link->setTextInteractionFlags(Qt::TextBrowserInteraction);
        QPalette lp = link->palette();
        lp.setColor(QPalette::Link, Qt::white);
        lp.setColor(QPalette::LinkVisited, Qt::white);
        link->setPalette(lp);

        auto *close = new QPushButton("Close");
        close->setObjectName("seg");
        close->setCursor(Qt::PointingHandCursor);
        QObject::connect(close, &QPushButton::clicked, &d, &QDialog::accept);

        lay->addWidget(logo);
        lay->addSpacing(6);
        lay->addWidget(name);
        lay->addWidget(by);
        lay->addSpacing(10);
        lay->addWidget(link);
        lay->addSpacing(14);
        lay->addWidget(close, 0, Qt::AlignCenter);

        d.exec();
    }

    void showOsdSettings()
    {
        QDialog d(&win);
        d.setWindowTitle("Overlay widgets");
        d.resize(600, 580);

        auto *outer = new QVBoxLayout(&d);
        outer->setContentsMargins(24, 20, 24, 20);
        outer->setSpacing(14);

        auto *title = new QLabel("Overlay widgets");
        title->setObjectName("hname");
        outer->addWidget(title);

        auto *sa = new QScrollArea;
        sa->setWidgetResizable(true);
        sa->setFrameShape(QFrame::NoFrame);
        sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        outer->addWidget(sa, 1);

        auto makeSwitch = [&d](bool on, std::function<void(bool)> apply) {
            auto *b = new QPushButton(on ? "ON" : "OFF");
            b->setObjectName("seg");
            b->setCheckable(true);
            b->setChecked(on);
            b->setMinimumWidth(64);
            b->setCursor(Qt::PointingHandCursor);
            QObject::connect(b, &QPushButton::toggled, &d, [b, apply](bool v) {
                b->setText(v ? "ON" : "OFF");
                apply(v);
            });
            return b;
        };
        auto caption = [](const QString &t) {
            auto *l = new QLabel(t);
            l->setObjectName("ckey");
            return l;
        };

        std::function<void()> rebuild;
        rebuild = [&] {
            auto *list = new QWidget;
            auto *lay = new QVBoxLayout(list);
            lay->setContentsMargins(0, 0, 0, 0);
            lay->setSpacing(12);

            for (int i = 0; i < osdCfg.size(); ++i) {
                const OsdCfg c = osdCfg[i];
                auto *card = new QFrame;
                card->setObjectName("card");
                auto *g = new QGridLayout(card);
                g->setContentsMargins(18, 16, 18, 18);
                g->setHorizontalSpacing(18);
                g->setVerticalSpacing(12);
                g->setColumnStretch(1, 1);

                auto *name = new QLabel(QString("Widget %1").arg(i + 1));
                name->setObjectName("name");
                auto *rm = new QPushButton("Remove");
                rm->setObjectName("seg");
                rm->setCursor(Qt::PointingHandCursor);
                QObject::connect(rm, &QPushButton::clicked, &d, [this, i, &rebuild] {
                    osdCfg.removeAt(i);
                    commitOsd();
                    rebuild();
                });
                g->addWidget(name, 0, 0, 1, 2);
                g->addWidget(rm, 0, 2, Qt::AlignRight);

                g->addWidget(caption("Show"), 1, 0);
                g->addWidget(makeSwitch(c.enabled, [this, i](bool v) {
                                 osdCfg[i].enabled = v;
                                 commitOsd();
                             }), 1, 1, Qt::AlignLeft);

                g->addWidget(caption("GPU"), 2, 0);
                auto *combo = new QComboBox;
                if (gpus.isEmpty()) {
                    combo->addItem("No GPU detected");
                    combo->setEnabled(false);
                } else {
                    for (const Gpu &gp : gpus) {
                        int same = 0;
                        for (const Gpu &o : gpus)
                            if (displayName(o) == displayName(gp))
                                ++same;
                        combo->addItem(same > 1 ? displayName(gp) + "  ·  " + slotShort(gp) : displayName(gp),
                                       gp.id);
                    }
                    const int at = c.gpuId.isEmpty() ? 0 : combo->findData(c.gpuId);
                    combo->setCurrentIndex(at < 0 ? 0 : at);
                    QObject::connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), &d,
                                     [this, i, combo](int k) {
                                         osdCfg[i].gpuId = combo->itemData(k).toString();
                                         commitOsd();
                                     });
                }
                g->addWidget(combo, 2, 1, 1, 2);

                g->addWidget(caption("Position"), 3, 0);
                static const char *const corners[4] = {"Top left", "Top right", "Bottom left", "Bottom right"};
                auto *pos = new QHBoxLayout;
                pos->setSpacing(6);
                auto *grp = new QButtonGroup(card);
                grp->setExclusive(true);
                for (int k = 0; k < 4; ++k) {
                    auto *b = new QPushButton(corners[k]);
                    b->setObjectName("seg");
                    b->setCheckable(true);
                    b->setChecked(c.corner == k);
                    b->setCursor(Qt::PointingHandCursor);
                    grp->addButton(b);
                    QObject::connect(b, &QPushButton::clicked, &d, [this, i, k] {
                        osdCfg[i].corner = k;
                        commitOsd();
                    });
                    pos->addWidget(b);
                }
                pos->addStretch();
                g->addLayout(pos, 3, 1, 1, 2);

                g->addWidget(caption("CPU & RAM"), 4, 0);
                g->addWidget(makeSwitch(c.sys, [this, i](bool v) {
                                 osdCfg[i].sys = v;
                                 commitOsd();
                             }), 4, 1, Qt::AlignLeft);

                lay->addWidget(card);
            }
            if (osdCfg.isEmpty()) {
                auto *none = new QLabel("No widgets. Add one below.");
                none->setObjectName("sub");
                lay->addWidget(none);
            }
            lay->addStretch();

            if (QWidget *old = sa->takeWidget()) {
                old->hide();
                old->deleteLater();
            }
            sa->setWidget(list);
        };
        rebuild();

        auto *foot = new QHBoxLayout;
        auto *add = new QPushButton("+ Add widget");
        add->setObjectName("seg");
        add->setCursor(Qt::PointingHandCursor);
        auto *close = new QPushButton("Close");
        close->setObjectName("seg");
        close->setCursor(Qt::PointingHandCursor);
        QObject::connect(add, &QPushButton::clicked, &d, [this, &rebuild] {
            addOsdWidget();
            rebuild();
        });
        QObject::connect(close, &QPushButton::clicked, &d, &QDialog::accept);
        foot->addWidget(add);
        foot->addStretch();
        foot->addWidget(close);
        outer->addLayout(foot);

        d.exec();
    }

    QLabel *textCell(QGridLayout *g, const QString &key, int r, int c, int span)
    {
        auto *cell = new QWidget;
        auto *l = new QVBoxLayout(cell);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(6);
        auto *k = new QLabel(key);
        k->setObjectName("ckey");
        auto *v = new QLabel;
        v->setObjectName("cval");
        v->setWordWrap(true);
        v->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        v->setTextInteractionFlags(Qt::TextSelectableByMouse);
        setValue(v, "N/A");
        l->addWidget(k);
        l->addWidget(v);
        g->addWidget(cell, r, c, 1, span);
        return v;
    }

    Metric barCell(QGridLayout *g, const QString &key, int r, int c, int valW)
    {
        auto *cell = new QWidget;
        auto *l = new QVBoxLayout(cell);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(6);
        auto *k = new QLabel(key);
        k->setObjectName("ckey");

        Metric m;
        m.v = new QLabel;
        m.v->setObjectName("cval");
        m.v->setFixedWidth(valW);
        setValue(m.v, "N/A");
        m.b = new BarWidget;

        auto *row = new QHBoxLayout;
        row->setSpacing(12);
        row->setContentsMargins(0, 0, TempBar::kOver, 0);
        row->addWidget(m.v, 0, Qt::AlignBottom);
        row->addWidget(m.b, 1);
        l->addWidget(k);
        l->addLayout(row);
        g->addWidget(cell, r, c);
        return m;
    }

    TempMetric tempCell(QGridLayout *g, const QString &key, int r, int valW)
    {
        auto *cell = new QWidget;
        auto *l = new QVBoxLayout(cell);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(6);
        auto *k = new QLabel(key);
        k->setObjectName("ckey");

        TempMetric m;
        m.v = new QLabel;
        m.v->setObjectName("cval");
        m.v->setFixedWidth(valW);
        setValue(m.v, "N/A");
        m.b = new TempBar;
        m.v->setContentsMargins(0, 0, 0, TempBar::kPad);

        auto *row = new QHBoxLayout;
        row->setSpacing(12);
        row->addWidget(m.v, 0, Qt::AlignBottom);
        row->addWidget(m.b, 1);
        l->addWidget(k);
        l->addLayout(row);
        g->addWidget(cell, r, 0, 1, 2);
        return m;
    }

    void applyTemp(TempMetric &m, bool has, double t, const QString &tip)
    {
        setValue(m.v, has ? QString("%1°C").arg(qRound(t)) : QString("N/A"));
        setLevel(m.v, has ? tempLevel(t) : "");
        m.b->setTemp(has, t);
        m.v->setToolTip(tip);
        m.b->setToolTip(tip);
    }

    RowUi makeRow(std::function<void()> onClick, std::function<void()> onInfo)
    {
        RowUi r;
        r.frame = new RowFrame;
        r.frame->setObjectName("slot");
        r.frame->setProperty("selected", false);
        r.frame->setCursor(Qt::PointingHandCursor);
        r.frame->setMinimumHeight(76);
        r.frame->onClick = std::move(onClick);

        auto *h = new QHBoxLayout(r.frame);
        h->setContentsMargins(14, 10, 14, 10);
        h->setSpacing(8);

        auto *col = new QVBoxLayout;
        col->setSpacing(2);
        r.name = new QLabel;
        r.name->setObjectName("name");
        r.name->setWordWrap(true);
        r.sub = new QLabel;
        r.sub->setObjectName("sub");
        r.live = new QLabel;
        r.live->setObjectName("live");
        col->addWidget(r.name);
        col->addWidget(r.sub);
        col->addWidget(r.live);
        h->addLayout(col, 1);

        auto *btn = new QPushButton("i");
        btn->setObjectName("info");
        btn->setToolTip("Info");
        QObject::connect(btn, &QPushButton::clicked, &win, [onInfo] { onInfo(); });
        h->addWidget(btn);
        return r;
    }

    void rebuildSidebar()
    {
        auto *list = new QWidget;
        auto *lay = new QVBoxLayout(list);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);

        rows.clear();
        for (const Gpu &g : gpus) {
            const QString id = g.id;
            RowUi r = makeRow([this, id] { select(id); },
                              [this, id] {
                                  if (const Gpu *p = find(id))
                                      showInfo(&win, *p);
                              });
            rows << r;
            lay->addWidget(r.frame);
        }
        if (gpus.isEmpty()) {
            auto *none = new QLabel(lact->isUp() ? "No GPUs reported by lactd" : "lactd not connected");
            none->setObjectName("sub");
            none->setContentsMargins(14, 14, 14, 14);
            none->setWordWrap(true);
            lay->addWidget(none);
        }
        lay->addStretch();

        if (QWidget *old = scroll->takeWidget()) {
            old->hide();
            old->deleteLater();
        }
        scroll->setWidget(list);

        for (int i = 0; i < rows.size(); ++i) {
            rows[i].frame->setProperty("selected", gpus[i].id == currentId);
            repolish(rows[i].frame);
            updateRow(i);
        }
    }

    void updateRow(int i)
    {
        if (i < 0 || i >= rows.size() || i >= gpus.size())
            return;
        const Gpu &g = gpus[i];
        RowUi &r = rows[i];
        r.name->setText(displayName(g));
        QStringList sub;
        if (!g.vendor.isEmpty())
            sub << g.vendor;
        if (!g.slot.isEmpty())
            sub << slotShort(g);
        r.sub->setText(sub.join("  ·  "));
        r.live->setText(g.haveStats ? makeView(g).brief : QString("-"));
    }

    void select(const QString &id)
    {
        currentId = id;
        for (int i = 0; i < rows.size() && i < gpus.size(); ++i) {
            rows[i].frame->setProperty("selected", gpus[i].id == id);
            repolish(rows[i].frame);
        }
        render();
    }

    void refreshHistory()
    {
        if (!hv)
            return;
        hv->setHistory(hist.value(currentId));
        hv->refresh();
    }

    // Everything recorded for one card since the app started.
    void showFull(const QString &id)
    {
        const Gpu *g = find(id);
        QDialog d(&win);
        d.setWindowTitle((g ? displayName(*g) : QString("GPU")) + " · history since app start");
        d.resize(1200, 820);

        auto *outer = new QVBoxLayout(&d);
        outer->setContentsMargins(0, 0, 0, 0);
        auto *sa = new QScrollArea;
        sa->setWidgetResizable(true);
        sa->setFrameShape(QFrame::NoFrame);
        auto *wrap = new QWidget;
        auto *wl = new QVBoxLayout(wrap);
        wl->setContentsMargins(24, 20, 24, 20);
        auto *view = new HistoryView(190, 2, 0, false, true);
        wl->addWidget(view);
        sa->setWidget(wrap);
        outer->addWidget(sa);

        view->setHistory(hist.value(id));
        view->refresh();
        auto *tm = new QTimer(&d);
        QObject::connect(tm, &QTimer::timeout, &d, [view] { view->refresh(); });
        tm->start(1000);
        d.exec();
    }

    void render()
    {
        const Gpu *g = find(currentId);
        hname->setText(g ? displayName(*g) : QString("-"));
        const View v = (g && g->haveStats) ? makeView(*g) : View();

        setValue(throttle, v.throttle);
        setValue(voltage, v.voltage);
        applyTemp(tgpu, v.hasGpuT, v.tGpu, v.tempTip);
        applyTemp(thot, v.hasHotT, v.tHot, v.tempTip);
        setValue(core.v, v.core);
        setValue(mem.v, v.mem);
        setValue(usage.v, v.usage);
        setValue(vram.v, v.vram);
        setValue(power.v, v.power);
        setValue(fan.v, v.fan);

        core.b->setData(v.coreR, v.coreE, v.coreS);
        mem.b->setData(v.memR, v.memE, v.memS);
        usage.b->setData(v.usageR, v.usageE, v.usageS);
        vram.b->setData(v.vramR, v.vramE, v.vramS);
        power.b->setData(v.powerR, v.powerE, v.powerS);
        fan.b->setData(v.fanR, v.fanE, v.fanS);

        vram.v->setToolTip(v.vramTip);
        power.v->setToolTip(v.powerTip);
        refreshHistory();
    }

    void setStatus(const QString &t)
    {
        status->setText(t);
        status->setVisible(!t.isEmpty());
    }

    Gpu *find(const QString &id)
    {
        for (Gpu &g : gpus)
            if (g.id == id)
                return &g;
        return nullptr;
    }

    int indexOf(const QString &id) const
    {
        for (int i = 0; i < gpus.size(); ++i)
            if (gpus[i].id == id)
                return i;
        return -1;
    }

    void onLactState(bool up, const QString &why)
    {
        if (up) {
            hint.clear();
            setStatus(QString());
            loadDevices();
            return;
        }
        setStatus(hint.isEmpty() ? "lactd unavailable · " + why + " · retrying" : hint);
        listing = false;
        for (int i = 0; i < gpus.size(); ++i) {
            gpus[i].busy = false;
            gpus[i].haveStats = false;
            updateRow(i);
        }
        render();
        updateOsd();
    }

    void ensureDaemon()
    {
        const int e = lact->lastErrno();
        if (e == EACCES || e == EPERM) {
            hint = "no permission for the lactd socket · add your user to the wheel/sudo group "
                   "or set daemon.admin_user in /etc/lact/config.yaml, then restart lactd";
            setStatus(hint);
            return;
        }
        if (startStage == 0)
            startService();
    }

    void startService()
    {
        startStage = 1;
        const QString pk = QStandardPaths::findExecutable("pkexec");
        const QString sc = QStandardPaths::findExecutable("systemctl");
        if (sc.isEmpty()) {
            startUserDaemon();
            return;
        }
        hint = "starting lactd · authorize the prompt";
        setStatus(hint);
        starter = new QProcess(&win);
        QObject::connect(starter, &QProcess::finished, &win, [this](int code, QProcess::ExitStatus st) {
            starter->deleteLater();
            starter = nullptr;
            if (code != 0 || st != QProcess::NormalExit)
                startUserDaemon();
            else
                hint.clear();
        });
        QObject::connect(starter, &QProcess::errorOccurred, &win, [this](QProcess::ProcessError err) {
            if (err == QProcess::FailedToStart) {
                starter->deleteLater();
                starter = nullptr;
                startUserDaemon();
            }
        });
        const QStringList args = {"enable", "--now", "lactd.service"};
        if (!pk.isEmpty())
            starter->start(pk, QStringList{sc} + args);
        else
            starter->start(sc, args);
    }

    void startUserDaemon()
    {
        if (startStage >= 2)
            return;
        startStage = 2;
        const QString exe = QStandardPaths::findExecutable("lact");
        if (exe.isEmpty()) {
            hint = "lactd is not running and could not be started · run: sudo systemctl enable --now lactd";
            setStatus(hint);
            return;
        }
        hint = "lactd service unavailable · running a local lact daemon";
        setStatus(hint);
        userDaemon = new QProcess(&win);
        userDaemon->start(exe, QStringList{"daemon"});
    }

    void poll()
    {
        if (osdWantsSys()) {
            sampleSystem();
            updateOsd();
        } else {
            cpuPrevTotal = 0;                      // forget the old sample so the next delta is fresh
            sysCpu = -1;
        }
        if (!lact->isUp()) {
            if (!lact->open())
                ensureDaemon();
            return;
        }
        if (gpus.isEmpty()) {
            loadDevices();
            return;
        }
        pollStats();
    }

    void pollStats()
    {
        ++tick;
        for (int i = 0; i < gpus.size(); ++i) {
            if (gpus[i].hwCore <= 0 && gpus[i].clockTries < 8 && tick % 5 == 0)
                fetchClocks(gpus[i].id);
            if (gpus[i].busy)
                continue;
            gpus[i].busy = true;
            const QString id = gpus[i].id;
            lact->request("device_stats", QJsonObject{{"id", id}},
                          [this, id](bool ok, const QJsonValue &d, const QString &) { onStats(id, ok, d); });
        }
    }

    void onStats(const QString &id, bool ok, const QJsonValue &d)
    {
        const int i = indexOf(id);
        if (i < 0)
            return;
        Gpu &g = gpus[i];
        g.busy = false;
        if (ok && d.isObject()) {
            g.stats = d.toObject();
            g.haveStats = true;
            double x = 0;
            if (numAt(d, "clockspeed.gpu_clockspeed", x) || numAt(d, "clockspeed.current_gfxclk", x))
                g.maxCore = qMax(g.maxCore, x);
            if (numAt(d, "clockspeed.vram_clockspeed", x))
                g.maxMem = qMax(g.maxMem, x);
            std::shared_ptr<History> &h = hist[id];
            if (!h)
                h = std::make_shared<History>();
            h->add(uptime.elapsed() / 1000.0, readingsOf(g));
        } else {
            g.haveStats = false;
        }
        updateRow(i);
        updateOsd();
        if (id == currentId)
            render();
    }

    void loadDevices()
    {
        if (listing)
            return;
        listing = true;
        lact->request("list_devices", QJsonObject(),
                      [this](bool ok, const QJsonValue &d, const QString &err) {
            listing = false;
            if (!ok) {
                if (lact->isUp())
                    setStatus("lactd: " + err);
                return;
            }

            QList<Gpu> fresh;
            auto add = [&](const QString &id, const QString &name) {
                if (id.isEmpty())
                    return;
                Gpu g;
                if (const Gpu *old = find(id))
                    g = *old;
                g.id = id;
                g.busy = false;
                parseId(g);
                if (!name.isEmpty())
                    setName(g, name);
                if (g.vendor.isEmpty())
                    g.vendor = vendorFromId(g.vendorId);
                fresh << g;
            };
            if (d.isArray()) {
                const QJsonArray arr = d.toArray();
                for (const QJsonValue &e : arr) {
                    if (e.isString())
                        add(e.toString(), QString());
                    else if (e.isObject())
                        add(e.toObject().value("id").toString(), e.toObject().value("name").toString());
                }
            } else if (d.isObject()) {
                const QJsonObject o = d.toObject();
                for (auto it = o.begin(); it != o.end(); ++it)
                    add(it.key(), it.value().toString());
            }

            gpus = fresh;
            if (indexOf(currentId) < 0)
                currentId = gpus.isEmpty() ? QString() : gpus.first().id;
            rebuildSidebar();
            render();
            updateOsd();

            for (const Gpu &g : gpus)
                fetchStatic(g.id);
            pollStats();
        });
    }

    void fetchStatic(const QString &id)
    {
        lact->request("device_info", QJsonObject{{"id", id}},
                      [this, id](bool ok, const QJsonValue &d, const QString &) {
            const int i = indexOf(id);
            if (i < 0 || !ok || !d.isObject())
                return;
            applyInfo(gpus[i], d.toObject());
            updateRow(i);
            if (id == currentId)
                render();
        });
        fetchClocks(id);
    }

    void applyHwMax(int i, double core, double mem)
    {
        Gpu &g = gpus[i];
        if (core > 0 && core < 20000)
            g.hwCore = qMax(g.hwCore, core);
        if (mem > 0 && mem < 20000)
            g.hwMem = qMax(g.hwMem, mem);
        g.maxCore = qMax(g.maxCore, g.hwCore);
        g.maxMem = qMax(g.maxMem, g.hwMem);
        if (g.id == currentId)
            render();
    }

    void fetchClocks(const QString &id)
    {
        const int gi = indexOf(id);
        if (gi < 0)
            return;
        gpus[gi].clockTries++;
        lact->request("device_clocks_info", QJsonObject{{"id", id}},
                      [this, id](bool ok, const QJsonValue &d, const QString &) {
            const int i = indexOf(id);
            if (i < 0 || !ok)
                return;
            static const QStringList coreKeys = {"max_sclk", "max_gpu_clock", "max_core_clock",
                                                 "max_graphics_clock", "gpu_clock_max", "sclk_max",
                                                 "gpu_clock_range", "gt_freq", "rp0_freq"};
            static const QStringList memKeys = {"max_mclk", "max_memory_clock", "max_vram_clock",
                                                "mem_clock_max", "mclk_max", "vram_clock_max",
                                                "vram_clock_range"};
            applyHwMax(i, findMax(d, coreKeys), findMax(d, memKeys));
        });
        lact->request("get_power_states", QJsonObject{{"id", id}},
                      [this, id](bool ok, const QJsonValue &d, const QString &) {
            const int i = indexOf(id);
            if (i < 0 || !ok)
                return;
            applyHwMax(i, maxStateValue(d, "core"), maxStateValue(d, "vram"));
        });
    }

    LactClient *lact = nullptr;
    QList<Gpu> gpus;
    QList<RowUi> rows;
    QString currentId;
    bool listing = false;
    QString hint;
    int tick = 0;
    QElapsedTimer uptime;
    QHash<QString, std::shared_ptr<History>> hist;
    HistoryView *hv = nullptr;
    int startStage = 0;
    QProcess *starter = nullptr;
    QProcess *userDaemon = nullptr;

    QList<OsdCfg> osdCfg;
    OsdHost *osdHost = nullptr;
    double sysCpu = -1;            // percent, -1 = not known yet
    double sysRamMiB = -1;
    quint64 cpuPrevTotal = 0, cpuPrevIdle = 0;

    QScrollArea *scroll = nullptr;
    SlidePane *sidebar = nullptr;
    QVariantAnimation *sidebarAnim = nullptr;
    bool sidebarOpen = true;
    QFrame *vline = nullptr;
    QLabel *hname = nullptr;
    QLabel *status = nullptr;
    QLabel *throttle = nullptr;
    QLabel *voltage = nullptr;
    TempMetric tgpu, thot;
    Metric core, mem, usage, vram, power, fan;

    QMainWindow win;
};

int main(int argc, char *argv[])
{
    // The overlay widgets are real desktop windows that must sit in a screen corner and stay
    // above other windows. Native Wayland lets a client do neither, so prefer X11 (XWayland
    // on a Wayland session). Set QT_QPA_PLATFORM yourself to override.
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "xcb;wayland");

    QApplication app(argc, argv);
    QGuiApplication::setDesktopFileName("gpu-panel");
    app.setWindowIcon(makeLogo());

    QFont font;
    font.setFamilies({"JetBrains Mono", "IBM Plex Mono", "DejaVu Sans Mono", "Liberation Mono", "monospace"});
    font.setStyleHint(QFont::Monospace);
    app.setFont(font);
    app.setStyleSheet(kStyle);

    Panel panel;
    panel.show();
    return app.exec();
}
