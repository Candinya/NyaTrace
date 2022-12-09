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
        PluginParameter {
            name: "osm.mapping.highdpi_tiles"
            value: true
        }
    }

    // 地图
    Map {
        id: map
        plugin: osmPlugin
        anchors.fill: parent
        center: QtPositioning.coordinate(36, 120)
        smooth: true
        zoomLevel: 2
        layer.enabled: true
        layer.samples: 8

    }

    // 连接各个追踪点的线
    MapPolyline {
        id: tracingLine
        line.width: 3
        line.color: "#ff7163"
        path: []
    }

    MapQuickItem {
        id: mapTooltip
        visible: false
        z: 1 // 放在最前面
        anchorPoint: Qt.point(-8, 8)
        sourceItem: Text {
            text: ""
            color:"#242424"
            font.bold: true
            styleColor: "#ECECEC"
            style: Text.Outline
            font.pointSize: 12
        }
    }

    // 画一个追踪点

    property var drewPoints: [];
    function drawHopPoint(latitude, longitude, accuracyRadius) {
        if (drewPoints.findIndex((point) => point.latitude === latitude && point.longitude === longitude) !== -1) {
            // 已经画过了
            return;
        } else {
            // 没画过，可以画，标记一下
            drewPoints.push({
                latitude,
                longitude
            });
        }

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

        // 添加到地图
        map.addMapItemGroup(hopPoint);
    }

    // 给线添加一个点
    function connectLine(latitude, longitude) {
        console.log("Drawing pont into line:", latitude, longitude);
        tracingLine.addCoordinate(QtPositioning.coordinate(latitude, longitude));
    }

    // 前往某个区域，并显示提示信息
    function gotoCoordinate(latitude, longitude, zoomLevel, message) {
        const newPosition = QtPositioning.coordinate(latitude, longitude);
        map.center = newPosition;
        map.zoomLevel = zoomLevel;
        mapTooltip.sourceItem.text = message;
        mapTooltip.coordinate = newPosition;
        mapTooltip.visible = true;
    }

    // 自动调整地图大小
    function fitMap() {
        map.fitViewportToVisibleMapItems();
    }

    // 清空地图
    function clearMap() {
        // 清空地图上的物品
        map.clearMapItems();

        // 清空追踪线
        for (const c of tracingLine.path) {
            tracingLine.removeCoordinate(c);
        }

        // 把线加回地图
        map.addMapItem(tracingLine);

        // 隐藏提示信息
        mapTooltip.visible = false;

        // 把提示信息加回地图
        map.addMapItem(mapTooltip);

        // 清空已经画过的记录列表
        drewPoints.splice(0, drewPoints.length);
    }
}
