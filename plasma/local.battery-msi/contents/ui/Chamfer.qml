// The one shape of the MSI section: a rectangle with two cut corners (top-right, bottom-left).
import QtQuick
import QtQuick.Shapes

Item {
    id: root
    property color fillColor: "transparent"
    property color lineColor: Qt.rgba(1, 1, 1, 0.2)
    property real cut: 8
    property real lineWidth: 1
    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            fillColor: root.fillColor
            strokeColor: root.lineColor
            strokeWidth: root.lineWidth
            startX: 0.5; startY: 0.5
            PathLine { x: root.width - root.cut; y: 0.5 }
            PathLine { x: root.width - 0.5; y: root.cut }
            PathLine { x: root.width - 0.5; y: root.height - 0.5 }
            PathLine { x: root.cut; y: root.height - 0.5 }
            PathLine { x: 0.5; y: root.height - root.cut }
            PathLine { x: 0.5; y: 0.5 }
        }
    }
}
