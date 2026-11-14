/*  MSI laptop section for the Power & Battery popup: thermals, fans, performance mode, Cooler Boost, charge limit.
    Talks to the MControlCenter tray app over its session-bus interface (io.github.dmitry_s93.MControlCenter /Control). */
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.plasma5support as P5Support
import org.kde.kirigami as Kirigami

Item {
    id: msi

    readonly property string bus: "gdbus call --session -d io.github.dmitry_s93.MControlCenter -o /Control -m io.github.dmitry_s93.MControlCenter.Control."
    property var st: ({})
    readonly property bool online: st.mode !== undefined
    readonly property int cpu: st.cpuTemp !== undefined ? st.cpuTemp : 0
    readonly property int gpu: st.gpuTemp !== undefined ? st.gpuTemp : -1

    visible: online
    implicitHeight: online ? col.implicitHeight + Kirigami.Units.largeSpacing : 0
    implicitWidth: parent ? parent.width : 300

    function tempColor(t) { return t >= 85 ? "#ff4560" : t >= 72 ? "#ffb020" : t >= 55 ? "#3dff9a" : "#19e3ff" }
    function call(method, arg) { exec.connectSource(bus + method + (arg !== undefined ? " " + arg : "")) }
    function refresh() { exec.connectSource(bus + "State") }

    P5Support.DataSource {
        id: exec
        engine: "executable"
        onNewData: (source, data) => {
            disconnectSource(source)
            if (source.indexOf("Control.State") >= 0) {
                const m = /'(\{.*\})'/.exec(data.stdout || "")
                if (m) { try { msi.st = JSON.parse(m[1]) } catch (e) {} } else msi.st = ({})
            } else settle.restart()
        }
    }
    Timer { id: settle; interval: 600; onTriggered: msi.refresh() }
    Timer { interval: 1500; running: msi.visible || true; repeat: true; triggeredOnStart: true; onTriggered: msi.refresh() }

    ColumnLayout {
        id: col
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        anchors.leftMargin: Kirigami.Units.smallSpacing; anchors.rightMargin: Kirigami.Units.smallSpacing
        spacing: Kirigami.Units.smallSpacing

        Rectangle { Layout.fillWidth: true; height: 1; color: Kirigami.Theme.textColor; opacity: 0.15 }

        RowLayout {
            Layout.fillWidth: true
            Kirigami.Icon { source: "cpu"; implicitWidth: Kirigami.Units.iconSizes.smallMedium; implicitHeight: implicitWidth }
            PlasmaComponents3.Label { text: "MSI Crosshair 16 HX AI"; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight }
            PlasmaComponents3.ToolButton { icon.name: "configure"; onClicked: msi.call("ShowWindow"); PlasmaComponents3.ToolTip { text: "Open MControlCenter" } }
        }

        // live readouts
        RowLayout {
            Layout.fillWidth: true; spacing: Kirigami.Units.smallSpacing
            Repeater {
                model: [
                    { l: "CPU", v: msi.cpu + "°C", c: msi.tempColor(msi.cpu) },
                    { l: "GPU", v: msi.gpu > 0 ? msi.gpu + "°C" : "asleep", c: msi.gpu > 0 ? msi.tempColor(msi.gpu) : Kirigami.Theme.disabledTextColor },
                    { l: "CPU fan", v: msi.st.cpuFanPct + " %", c: Kirigami.Theme.highlightColor },
                    { l: "GPU fan", v: msi.st.gpuFanPct + " %", c: Kirigami.Theme.highlightColor }
                ]
                delegate: Rectangle {
                    required property var modelData
                    Layout.fillWidth: true; Layout.preferredWidth: 1
                    implicitHeight: cell.implicitHeight + Kirigami.Units.smallSpacing * 2
                    radius: Kirigami.Units.cornerRadius; color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.07)
                    border.width: 1; border.color: Qt.alpha(modelData.c, 0.5)
                    ColumnLayout {
                        id: cell; anchors.centerIn: parent; spacing: 0
                        PlasmaComponents3.Label { Layout.alignment: Qt.AlignHCenter; text: modelData.v; color: modelData.c; font.bold: true; font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.25 }
                        PlasmaComponents3.Label { Layout.alignment: Qt.AlignHCenter; text: modelData.l; opacity: 0.65; font.pixelSize: Kirigami.Theme.smallFont.pixelSize }
                    }
                }
            }
        }

        PlasmaComponents3.Label { text: "Performance mode"; opacity: 0.7; font.pixelSize: Kirigami.Theme.smallFont.pixelSize }
        RowLayout {
            Layout.fillWidth: true; spacing: Kirigami.Units.smallSpacing
            Repeater {
                model: [{ k: "turbo", n: "Turbo", c: "#ff2bd6" }, { k: "balanced", n: "Balanced", c: "#19e3ff" }, { k: "silent", n: "Silent", c: "#966eff" }, { k: "eco", n: "Eco", c: "#3dff9a" }]
                delegate: PlasmaComponents3.Button {
                    required property var modelData
                    Layout.fillWidth: true; Layout.preferredWidth: 1
                    text: modelData.n; checkable: true; autoExclusive: false; checked: msi.st.mode === modelData.k
                    onClicked: { checked = true; msi.call("SetMode", "'" + modelData.k + "'") }
                    background: Rectangle {
                        radius: Kirigami.Units.cornerRadius
                        color: parent.checked ? Qt.alpha(modelData.c, 0.22) : (parent.hovered ? Qt.rgba(1, 1, 1, 0.08) : "transparent")
                        border.width: parent.checked ? 2 : 1; border.color: parent.checked ? modelData.c : Qt.alpha(Kirigami.Theme.textColor, 0.3)
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            PlasmaComponents3.Label { text: "Cooler Boost (fans to maximum)"; Layout.fillWidth: true }
            PlasmaComponents3.Switch { checked: msi.st.coolerBoost === true; onToggled: msi.call("SetCoolerBoost", checked ? "true" : "false") }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: Kirigami.Units.smallSpacing
            PlasmaComponents3.Label { text: "Charge limit"; Layout.fillWidth: true }
            Repeater {
                model: [60, 80, 100]
                delegate: PlasmaComponents3.Button {
                    required property int modelData
                    text: modelData + " %"; checkable: true; autoExclusive: false; checked: msi.st.chargeLimit === modelData
                    onClicked: { checked = true; msi.call("SetChargeLimit", String(modelData)) }
                }
            }
        }
        Item { implicitHeight: Kirigami.Units.smallSpacing }
    }
}
