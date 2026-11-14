// LED-style segmented meter. `value` 0..1; segments are coloured by the value they stand for (calm until it matters).
import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: root
    property real value: 0
    property int segments: 20
    property bool temperature: true        // colour by temperature scale (30..100 °C) instead of a flat colour
    property color accent: "#e7e0cd"
    property color flat: Kirigami.Theme.textColor
    implicitHeight: 6
    Behavior on value { NumberAnimation { duration: 450; easing.type: Easing.OutCubic } }
    Row {
        anchors.fill: parent
        spacing: 2
        Repeater {
            model: root.segments
            Rectangle {
                required property int index
                readonly property real pos: (index + 0.5) / root.segments
                readonly property real lit: Math.max(0, Math.min(1, root.value * root.segments - index))
                width: (root.width - (root.segments - 1) * 2) / root.segments
                height: root.height
                color: lit > 0 ? Qt.alpha(root.temperature ? (30 + pos * 70 < 72 ? root.flat : 30 + pos * 70 < 86 ? root.accent : "#ff4632") : root.flat, 0.35 + 0.65 * lit)
                              : Qt.alpha(root.flat, 0.12)
            }
        }
    }
}
