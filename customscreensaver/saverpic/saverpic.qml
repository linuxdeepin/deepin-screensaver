/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     wangchunlin<wangchunlin@uniontech.com>
 *
 * Maintainer: wangchunlin<wangchunlin@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.11
import Qt.labs.platform 1.0
//sudo apt install qml-module-qt-labs-platform

Image {
    property string imagePath: StandardPaths.standardLocations(StandardPaths.HomeLocation) + "/.config/deepin/deepin-screensaver/saverpic/defaultpic.jpg"

    id: root
    anchors.fill: parent

    // If the source URL indicates a non-existing local file or resource, the Image element attempts to auto-detect the file extension.
    // If an existing file can be found by appending any of the supported image file extensions to the source URL, then that file will be loaded.
    source: imagePath

    // 图像将缩放以适应大小
    fillMode: Image.Stretch

    // 同步加载资源
    asynchronous: false

    // 忽略EXIF信息
    autoTransform: false

    // 居中显示
    horizontalAlignment: Image.AlignHCenter

    Component.onCompleted: {
        console.log(imagePath)
    }

    onStatusChanged: {
        if (root.status == Image.Null)
            console.log("no image has been set.")
        else if (root.status == Image.Ready)
            console.log("the image has been loaded.")
        else if (root.status == Image.Loading)
            console.log("the image is currently being loaded.")
        else if (root.status == Image.Error)
            console.log("an error occurred while loading the image.")
    }
}
