/* extcap_options_dialog.cpp
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <config.h>

#include <glib.h>

#include <extcap_options_dialog.h>
#include <ui_extcap_options_dialog.h>

#include <wireshark_application.h>

#ifdef HAVE_EXTCAP
#include <QMessageBox>
#include <QMap>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>

#include "ringbuffer.h"
#include "ui/capture_ui_utils.h"
#include "ui/capture_globals.h"
#include "ui/iface_lists.h"
#include "ui/last_open_dir.h"

#include "ui/ui_util.h"
#include "ui/util.h"
#include <wsutil/utf8_entities.h>

#include <cstdio>
#include <epan/addr_resolv.h>
#include <wsutil/filesystem.h>

#include <extcap.h>
#include <extcap_parser.h>

#include "qt_ui_utils.h"

#include <ui/qt/extcap_argument.h>
#include <ui/qt/extcap_argument_file.h>
#include <ui/qt/extcap_argument_multiselect.h>

ExtcapOptionsDialog::ExtcapOptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExtcapOptionsDialog),
    device_name(""),
    device_idx(0),
    device_defaults(NULL)
{
    ui->setupUi(this);

    setWindowTitle(wsApp->windowTitleString(tr("Extcap Interface Options")));

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Start"));

}

ExtcapOptionsDialog * ExtcapOptionsDialog::createForDevice(QString &dev_name, QWidget *parent)
{
    interface_t device;
    ExtcapOptionsDialog * resultDialog = NULL;
    bool dev_found = false;
    guint if_idx;

    if ( dev_name.length() == 0 )
        return NULL;

    for (if_idx = 0; if_idx < global_capture_opts.all_ifaces->len; if_idx++)
    {
        device = g_array_index(global_capture_opts.all_ifaces, interface_t, if_idx);
        if (dev_name.compare(QString(device.name)) == 0 && device.if_info.type == IF_EXTCAP)
        {
            dev_found = true;
            break;
        }
    }

    if ( ! dev_found )
        return NULL;

    resultDialog = new ExtcapOptionsDialog(parent);
    resultDialog->device_name = QString(dev_name);
    resultDialog->device_idx = if_idx;
    resultDialog->device_defaults = device.external_cap_args_settings;

    resultDialog->setWindowTitle(wsApp->windowTitleString(tr("Extcap Interface Options") + ": " + device.display_name));

    resultDialog->updateWidgets();

    /* mark required fields */
    resultDialog->anyValueChanged();

    return resultDialog;
}


ExtcapOptionsDialog::~ExtcapOptionsDialog()
{
    delete ui;
}

void ExtcapOptionsDialog::on_buttonBox_accepted()
{
    if (saveOptionToCaptureInfo()) {
        accept();
    }
}

void ExtcapOptionsDialog::anyValueChanged()
{
    bool allowStart = true;

    ExtcapArgumentList::const_iterator iter;

    /* All arguments are being iterated, to ensure, that any error handling catches all arguments */
    for(iter = extcapArguments.constBegin(); iter != extcapArguments.constEnd(); ++iter)
    {
        /* The dynamic casts are necessary, because we come here using the Signal/Slot system
         * of Qt, and -in short- Q_OBJECT classes cannot be multiple inherited. Another possibility
         * would be to use Q_INTERFACE, but this causes way more nightmares, and we really just
         * need here an explicit cast for the check functionality */
        if ( dynamic_cast<ExtArgBool *>((*iter)) != NULL)
        {
            if ( ! ((ExtArgBool *)*iter)->isValid() )
                allowStart = false;
        }
        else if ( dynamic_cast<ExtArgRadio *>((*iter)) != NULL)
        {
            if ( ! ((ExtArgRadio *)*iter)->isValid() )
                allowStart = false;
        }
        else if ( dynamic_cast<ExtArgSelector *>((*iter)) != NULL)
        {
            if ( ! ((ExtArgSelector *)*iter)->isValid() )
                allowStart = false;
        }
        else if ( dynamic_cast<ExtArgMultiSelect *>((*iter)) != NULL)
        {
            if ( ! ((ExtArgMultiSelect *)*iter)->isValid() )
                allowStart = false;
        }
        else if ( dynamic_cast<ExtcapArgumentFileSelection *>((*iter)) != NULL)
        {
            if ( ! ((ExtcapArgumentFileSelection *)*iter)->isValid() )
                allowStart = false;
        }
        else if ( dynamic_cast<ExtArgNumber *>((*iter)) != NULL)
        {
            if ( ! ((ExtArgNumber *)*iter)->isValid() )
                allowStart = false;
        }
        else if ( dynamic_cast<ExtArgText *>((*iter)) != NULL)
        {
            if ( ! ((ExtArgText *)*iter)->isValid() )
                allowStart = false;
        }
        else
            if ( ! (*iter)->isValid() )
                allowStart = false;
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(allowStart);
}

void ExtcapOptionsDialog::updateWidgets()
{
    GList * arguments = NULL, * walker = NULL, * item = NULL;
    QWidget * lblWidget = NULL, *editWidget = NULL;
    ExtcapArgument * argument = NULL;
    bool allowStart = true;

    unsigned int counter = 0;

    if ( device_name.length() == 0  )
        return;

    arguments = extcap_get_if_configuration((const char *)( device_name.toStdString().c_str() ) );
    walker = g_list_first(arguments);

    QGridLayout * layout = new QGridLayout();

    ExtcapArgumentList required;
    ExtcapArgumentList optional;

    while ( walker != NULL )
    {
        item = g_list_first((GList *)(walker->data));
        while ( item != NULL )
        {
            argument = ExtcapArgument::create((extcap_arg *)(item->data), device_defaults);
            if ( argument != NULL )
            {
                if ( argument->isRequired() )
                    required << argument;
                else
                    optional << argument;

            }
            item = item->next;
        }
        walker = walker->next;
    }

    if ( required.length() > 0 )
        extcapArguments << required;

    if ( optional.length() > 0 )
        extcapArguments << optional;

    ExtcapArgumentList::iterator iter = extcapArguments.begin();
    while ( iter != extcapArguments.end() )
    {
        lblWidget = (*iter)->createLabel((QWidget *)this);
        if ( lblWidget != NULL )
        {
            layout->addWidget(lblWidget, counter, 0, Qt::AlignVCenter);
            editWidget = (*iter)->createEditor((QWidget *) this);
            if ( editWidget != NULL )
            {
                layout->addWidget(editWidget, counter, 1, Qt::AlignVCenter);
            }

            if ( (*iter)->isRequired() && ! (*iter)->isValid() )
                allowStart = false;

            connect((*iter), SIGNAL(valueChanged()), this, SLOT(anyValueChanged()));

            counter++;
        }
        ++iter;
    }

    if ( counter > 0 )
    {
        setStyleSheet ( "QLabel[isRequired=\"true\"] { font-weight: bold; } ");

        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(allowStart);

        ui->verticalLayout->addLayout(layout);
        ui->verticalLayout->addSpacerItem(new QSpacerItem(20, 100, QSizePolicy::Minimum, QSizePolicy::Expanding));
    }
    else
    {
        delete layout;
    }
}

// Not sure why we have to do this manually.
void ExtcapOptionsDialog::on_buttonBox_rejected()
{
    if (saveOptionToCaptureInfo()) {
        reject();
    }
}

void ExtcapOptionsDialog::on_buttonBox_helpRequested()
{
    // Probably the wrong URL.
    wsApp->helpTopicAction(HELP_EXTCAP_OPTIONS_DIALOG);
}

bool ExtcapOptionsDialog::saveOptionToCaptureInfo()
{
    GHashTable * ret_args;
    interface_t device;

    device = g_array_index(global_capture_opts.all_ifaces, interface_t, device_idx);
    global_capture_opts.all_ifaces = g_array_remove_index(global_capture_opts.all_ifaces, device_idx);

    ret_args = g_hash_table_new(g_str_hash, g_str_equal);

    ExtcapArgumentList::const_iterator iter;

    for(iter = extcapArguments.constBegin(); iter != extcapArguments.constEnd(); ++iter)
    {
        QString call = (*iter)->call();
        QString value = (*iter)->value();

        if ((*iter)->argument()->arg_type != EXTCAP_ARG_BOOLFLAG && value.length() == 0)
            continue;

        if ( call.length() <= 0 )
            continue;

        if ( value.compare((*iter)->defaultValue()) == 0 )
            continue;

        gchar * call_string = g_strdup(call.toStdString().c_str());
        gchar * value_string = g_strdup(value.toStdString().c_str());

        g_hash_table_insert(ret_args, call_string, value_string );
    }

    if (device.external_cap_args_settings != NULL)
      g_hash_table_unref(device.external_cap_args_settings);
    device.external_cap_args_settings = ret_args;

    g_array_insert_val(global_capture_opts.all_ifaces, device_idx, device);

    return true;
}

#endif /* HAVE_LIBPCAP */

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
