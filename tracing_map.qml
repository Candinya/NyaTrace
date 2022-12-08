import QtQuick 2.15
import QtLocation 5.15
import QtPositioning 5.15

Rectangle {
    id: container
    anchors.fill: parent

    // 参考 https://doc.qt.io/qt-5/location-plugin-osm.html
    Plugin {
        id: osmPlugin
        name: "osm"
        PluginParameter {
            name: "osm.useragent"
            value: "NyaTrace"
        }
    }

    // 地图
    Map {
        id: map
        plugin: osmPlugin
        anchors.fill: parent
        center: QtPositioning.coordinate(34.7732, 113.722)
        zoomLevel: 2
        layer.enabled: true
        layer.samples: 8

        // 连接各个追踪点的线
        MapPolyline {
            id: tracingLine
            line.width: 3
            line.color: "#ff7163"
            path: []
        }

    }

    // 画一个追踪点
    function drawHopPoint(latitude, longitude, accuracyRadius, hop, message) {
        // 画组
        const hopPoint = Qt.createQmlObject(`
            import QtQuick 2.15
            import QtLocation 5.15
            import QtPositioning 5.15

            MapItemGroup {
                id: hopGroup
                property real latitude: 34.7732
                property real longitude: 113.722
                property real accuracyRadius: 1000
                property string message: ""

                property string themeColor: "#ff7163"

                MapQuickItem {
                    sourceItem: Rectangle {
                        width: 14
                        height: 14
                        color: hopGroup.themeColor
                        border.width: 2
                        border.color: "white"
                        smooth: true
                        radius: 7
                    }
                    coordinate: QtPositioning.coordinate(hopGroup.latitude, hopGroup.longitude)
                    opacity: 1.0
                    anchorPoint: Qt.point(sourceItem.width/2, sourceItem.height/2)
                }

                MapQuickItem {
                    sourceItem: Text {
                        text: hopGroup.message
                        color:"#242424"
                        font.bold: true
                        styleColor: "#ECECEC"
                        style: Text.Outline
                        font.pointSize: 15
                    }
                    coordinate: QtPositioning.coordinate(hopGroup.latitude, hopGroup.longitude)
                    anchorPoint: Qt.point(-8, 24)
                }

                MapCircle {
                    color: hopGroup.themeColor
                    opacity: 0.18
                    radius: hopGroup.accuracyRadius
                    center: QtPositioning.coordinate(hopGroup.latitude, hopGroup.longitude)
                    border.width: 3
                    border.color: hopGroup.themeColor
                }
            }
        `, map);

        // 设置组属性
        hopPoint.latitude = latitude
        hopPoint.longitude = longitude
        hopPoint.accuracyRadius = accuracyRadius
        hopPoint.message = message

        // 添加到地图
        map.addMapItemGroup(hopPoint);
    }

    // 给线添加一个点
    function connectLine(latitude, longitude) {
        console.log("Drawing pont into line: ", latitude, longitude);
        tracingLine.addCoordinate(QtPositioning.coordinate(latitude, longitude));
    }

    // 自动调整地图大小
    function fitMap() {
        map.fitViewportToVisibleMapItems();
    }

    // 清空地图
    function clearMap() {
        tracingLine.path = []; // 直接清空
        map.clearMapItems();
    }
}
