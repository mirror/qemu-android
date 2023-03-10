// Copyright (C) 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/error-dialog.h"

#include <qmessagebox.h>  // for QMessageBox::Icon, QMessag...
#include <QMessageBox>    // for QMessageBox
#include <functional>     // for __base
#include <tuple>          // for tuple

#include "host-common/hw-config.h"
#include "android/console.h"
#include "aemu/base/memory/OnDemand.h"  // for OnDemand
#include "android/cmdline-option.h"        // for android_cmdLineOptions
#include "android/utils/debug.h"

class QString;
class QWidget;

using Dialog = android::base::AtomicMemberOnDemandT<QMessageBox,
                                                    QMessageBox::Icon,
                                                    QString,
                                                    QString,
                                                    QMessageBox::StandardButton,
                                                    QWidget*>;

static Dialog* sErrorDialog = nullptr;

void initErrorDialog(QWidget* parent) {
    // This dialog will be deleted when the parent is deleted, which occurs
    // when the emulator closes.
    if (!sErrorDialog) {
        sErrorDialog = new Dialog(QMessageBox::Warning, "", "", QMessageBox::Ok,
                                  parent);
    }
}

void showErrorDialog(const QString& message, const QString& title) {
    // b/194152586 Showing error dialog in embedded mode caused
    // emulator to crash. To workaround, print to terminal instead
    // when running as embedded emulator.
    if (getConsoleAgents()
                ->settings->android_cmdLineOptions()
                ->qt_hide_window) {
        derror("%s : %s", title.toStdString().c_str(), message.toStdString().c_str());
    } else if (sErrorDialog) {
        sErrorDialog->get().setModal(true);
        sErrorDialog->get().setText(message);
        sErrorDialog->get().setWindowTitle(title);
        sErrorDialog->get().exec();
    }
}

void deleteErrorDialog() {
    delete sErrorDialog;
    sErrorDialog = nullptr;
}
