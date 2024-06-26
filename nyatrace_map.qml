import QtQuick
import QtLocation
import QtPositioning

Rectangle {
    id: container
    anchors.fill: parent

    // 参考 https://doc.qt.io/qt-6/location-plugin-osm.html
    Plugin {
        id: osmPlugin
        name: "osm"
        PluginParameter {
            name: "osm.useragent"
            value: "NyaTrace"
        }

        // 是否使用高分辨率地图（感觉没什么用，还徒增网络请求，所以注释掉了）
        // PluginParameter {
        //     name: "osm.mapping.highdpi_tiles"
        //     value: true
        // }
    }

    // 地图
    MapView {
        id: view
        map.plugin: osmPlugin
        anchors.fill: parent
        map.center: QtPositioning.coordinate(36, 120)
        smooth: true
        map.zoomLevel: 2
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

    // 提示文本
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

    // 共享追踪点的 QML 组件
    property var mapPointComponent;
    function loadMapPointComponent() {
        console.log("mapPointComponent loading");
        mapPointComponent = Qt.createComponent("nyatrace_map_point.qml");
    }

    Component.onCompleted: loadMapPointComponent();

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
        const mapPointObject = mapPointComponent.createObject(view.map);

        // 设置组属性
        mapPointObject.latitude = latitude;
        mapPointObject.longitude = longitude;
        mapPointObject.accuracyRadius = accuracyRadius;

        // 添加到地图
        view.map.addMapItemGroup(mapPointObject);
    }

    // 给线添加一个点
    function connectLine(latitude, longitude) {
        console.log("Drawing pont into line:", latitude, longitude);
        tracingLine.addCoordinate(QtPositioning.coordinate(latitude, longitude));
    }

    // 前往某个区域，并显示提示信息
    function gotoCoordinate(latitude, longitude, zoomLevel, message) {
        const newPosition = QtPositioning.coordinate(latitude, longitude);
        view.map.center = newPosition;
        view.map.zoomLevel = zoomLevel;
        mapTooltip.sourceItem.text = message;
        mapTooltip.coordinate = newPosition;
        mapTooltip.visible = true;
    }

    // 自动调整地图大小
    function fitMap() {
        view.map.fitViewportToVisibleMapItems();
    }

    // 清空地图
    function clearMap() {
        // 清空地图上的物品
        view.map.clearMapItems();

        // 清空追踪线
        for (const c of tracingLine.path) {
            tracingLine.removeCoordinate(c);
        }

        // 把线加回地图
        view.map.addMapItem(tracingLine);

        // 隐藏提示信息
        mapTooltip.visible = false;

        // 把提示信息加回地图
        view.map.addMapItem(mapTooltip);

        // 清空已经画过的记录列表
        drewPoints.splice(0, drewPoints.length);
    }
}
