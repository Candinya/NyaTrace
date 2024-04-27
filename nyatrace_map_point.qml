import QtQuick
import QtLocation
import QtPositioning

MapItemGroup {
    id: pointGroup
    property real latitude: 34.7732
    property real longitude: 113.722
    property real accuracyRadius: 1000

    property string themeColor: "#ff7163"

    MapQuickItem {
        sourceItem: Rectangle {
            width: 14
            height: 14
            color: pointGroup.themeColor
            border.width: 2
            border.color: "white"
            smooth: true
            radius: 7
        }
        coordinate: QtPositioning.coordinate(pointGroup.latitude, pointGroup.longitude)
        opacity: 1.0
        anchorPoint: Qt.point(sourceItem.width/2, sourceItem.height/2)
    }

    MapCircle {
        color: pointGroup.themeColor
        opacity: 0.18
        radius: pointGroup.accuracyRadius
        center: QtPositioning.coordinate(pointGroup.latitude, pointGroup.longitude)
        border.width: 3
        border.color: pointGroup.themeColor
    }
}
