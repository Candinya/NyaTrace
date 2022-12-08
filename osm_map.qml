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

    Map {
        id: map
        plugin: osmPlugin
        anchors.fill: parent
        center: QtPositioning.coordinate(34.7732, 113.722)
        zoomLevel: 14
        layer.enabled: true
        layer.samples: 8

    }

    MapItemGroup {
        id: testGroup
        property real latitude: 34.7732
        property real longitude: 113.722
        property real accuracyRadius: 1000
        property string message: "Point to Another x"

        property string themeColor: "#e41e25"

        property alias position: poiAnother.coordinate

        MapQuickItem {
            id: poiAnother
            sourceItem: Rectangle { width: 14; height: 14; color: testGroup.themeColor; border.width: 2; border.color: "white"; smooth: true; radius: 7 }
            coordinate: QtPositioning.coordinate(testGroup.latitude, testGroup.longitude)
            opacity: 1.0
            anchorPoint: Qt.point(sourceItem.width/2, sourceItem.height/2)
        }

        MapQuickItem {
            sourceItem: Text{
                text: testGroup.message
                color:"#242424"
                font.bold: true
                styleColor: "#ECECEC"
                style: Text.Outline
            }
            coordinate: poiAnother.coordinate
            anchorPoint: Qt.point(-poiAnother.sourceItem.width * 0.5,poiAnother.sourceItem.height * 1.5)
        }

        MapCircle {
            color: testGroup.themeColor
            opacity: 0.18
            radius: testGroup.accuracyRadius
            center: poiAnother.coordinate
            border.width: 3
            border.color: testGroup.themeColor
        }
    }

    property MapCircle testScale2

    function ping(pingMsg) {
        map.addMapItemGroup(testGroup);

//        testScale2 = Qt.createComponent('import QtQuick 2.15; import QtLocation 5.15; import QtPositioning 5.15; MapCircle { color: "#e41e25"; opacity: 0.18; radius: 1000; center: QtPositioning.coordinate(34.7732, 113.722); border.width: 3; border.color: "#e41e25"; }', map);

//        map.addMapItem(testScale2);

        console.log(pingMsg);
        return "pong";
    }

    function drawHopData(latitude, longitude, accuracyRadius, hop, message) {
        // 画中心点
        const currentHopCenter = Qt.createComponent('import QtQuick 2.15; import QtLocation 5.15; MapQuickItem { sourceItem： Rectangle { width: 16; height: 16; color: "#e41e25"; border.width: 2; border.color: "white"; smooth: true; radius: 8 }; opacity: 1.0; anchorPoint: Qt.point(sourceItem.width/2, sourceItem.height/2) }', container);
        currentHopCenter.coordinate = QtPositioning.coordinate(latitude, longitude);

        // 画范围圈
        const currentHopScope = Qt.createComponent('import QtLocation 5.15; MapCircle { smooth: true; color: "#e41e25"; opacity: 0.6; border: { width: 3; color: "#e41e25"; }; }', container);
        currentHopScope.center = QtPositioning.coordinate(latitude, longitude);
        currentHopScope.radius = accuracyRadius;

//        // 画提示信息
//        currentHopMessageText = new Text({
//            text: message,
//            color:"#242424",
//            font: {
//                bold: true
//            },
//            styleColor: "#ECECEC",
//            style: Text.Outline
//        });
//        currentHopMessage = new MapQuickItem({
//            sourceItem: currentHopMessageText,
//            coordinate: currentHopCenter.coordinate,
//            anchorPoint: Qt.point(-currentHopCenterRectangle.width * 0.5, currentHopCenterRectangle.height * 1.5)
//        });

        // 添加到地图
        map.addMapItem(currentHopCenter);
        map.addMapItem(currentHopScope);
//        map.addMapItem(currentHopMessage);
    }

    function fitMap() {
        map.fitViewportToVisibleMapItems();
    }

    function clearMap() {
        map.clearMapItems();
    }
}
