#include "controls_widget.h"

#include <marsyas/system/MarControl.h>

#include <QDebug>
#include <QVBoxLayout>

using namespace Marsyas;

typedef std::map<std::string, MarControlPtr> control_map_t;

static QVariant variantFromControl ( const MarControlPtr & control )
{
  QVariant value;

  std::string type = control->getType();
  if (type == "mrs_real")
    value = QString::number( control->to<mrs_real>() );
  else if (type == "mrs_natural")
    value = QString::number( control->to<mrs_natural>() );
  else if (type == "mrs_bool")
  {
    value = control->to<mrs_bool>() ? QString("true") : QString("false");
  }
  else if (type == "mrs_string")
    value = QString::fromStdString(control->to<mrs_string>());
  else if (type == "mrs_realvec")
    value = QString("[...]");
  else
    value = QString("<unknown>");

  return value;
}

ControlsWidget::ControlsWidget( QWidget * parent):
  QWidget(parent),
  m_system(0)
{
  m_tree = new QTreeWidget;
  m_tree->setHeaderLabels( QStringList()  << "Name" << "Value" << "Type" );
  m_tree->setRootIsDecorated(false);

  QVBoxLayout *column = new QVBoxLayout;
  column->setContentsMargins(0,0,0,0);
  column->setSpacing(0);
  column->addWidget(m_tree);

  setLayout(column);

  connect(m_tree, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
          this, SLOT(onItemClicked(QTreeWidgetItem*,int)));
}

void ControlsWidget::setSystem( Marsyas::MarSystem * system )
{
  if (m_system == system)
    return;

  m_system = system;

  rebuild();

  if (m_system)
    emit pathChanged( QString::fromStdString(m_system->path()) );
  else
    emit pathChanged( QString() );
}

void ControlsWidget::rebuild()
{
  m_tree->clear();

  if (!m_system)
    return;

  control_map_t controls = m_system->controls();
  control_map_t::iterator it;
  for (it = controls.begin(); it != controls.end(); ++it)
  {
    QString path = QString::fromStdString( it->first );
    QStringList path_components = path.split('/');
    if (path_components.size() < 2) {
      qWarning() << "Anomalous control path:" << path;
      continue;
    }
    QString type = path_components[0];
    QString name = path_components[1];

    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setData(NameColumn, Qt::DisplayRole, name);
    item->setData(TypeColumn, Qt::DisplayRole, type);
    item->setData(PathColumn, Qt::DisplayRole, path);

    m_tree->addTopLevelItem(item);
  }

  refresh();

  m_tree->resizeColumnToContents(NameColumn);
  m_tree->resizeColumnToContents(TypeColumn);
  // NOTE: don't resize value column, as it may contain
  // ridiculously long strings.
}

void ControlsWidget::refresh()
{
  if (!m_system)
    return;

  int count = m_tree->topLevelItemCount();
  for (int item_idx = 0; item_idx < count; ++item_idx)
  {
    QTreeWidgetItem *item = m_tree->topLevelItem(item_idx);
    QString path = item->data( PathColumn, Qt::DisplayRole ).toString();
    MarControlPtr control = m_system->getControl( path.toStdString() );
    if (control.isInvalid()) {
      qWarning() << "Control path invalid:" << path;
      continue;
    }
    item->setData(ValueColumn, Qt::DisplayRole, variantFromControl(control));
  }
}

void ControlsWidget::onItemClicked( QTreeWidgetItem *item, int )
{
  QString path = item->data( PathColumn, Qt::DisplayRole ).toString();
  emit controlClicked(path);
}
