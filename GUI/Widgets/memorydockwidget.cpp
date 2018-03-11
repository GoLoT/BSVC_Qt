#include "memorydockwidget.h"
#include <QLayout>
#include <QHeaderView>
#include "mainwindow.h"

MemoryDockWidget::MemoryDockWidget(MainWindow* window, Address startAddress, bool closable, QWidget *parent) :
  QDockWidget("Memory", parent),
  window(window)
{
  //this->setLayout(new QVBoxLayout);
  if(closable)
    this->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
  else
    this->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
  memoryView = new QTableView();
  //this->layout()->addWidget(memoryView);
  if(closable)
    this->setObjectName(QStringLiteral("altMemoryDock"));
  else
    this->setObjectName(QStringLiteral("memoryDock"));
  this->setStyleSheet(QLatin1String("QDockWidget::title {\n"
"    text-align: left;\n"
"    background: transparent;\n"
"    padding-left: 5px;\n"
"}\n"
"\n"
"QWidget {\n"
"    border: 1px solid lightgrey;\n"
"}"));
  QWidget* dockContent = new QWidget();
  //dockContent->setObjectName(QStringLiteral("dockWidgetContents_4"));
  QGridLayout* layout = new QGridLayout(dockContent);
  layout->setSpacing(6);
  layout->setContentsMargins(11, 11, 11, 11);
  //layout->setObjectName(QStringLiteral("gridLayout_3"));
  memoryView = new QTableView(dockContent);
  if(closable)
    memoryView->setObjectName(QStringLiteral("memoryView"));
  memoryView->setFrameShape(QFrame::StyledPanel);
  memoryView->setShowGrid(true);
  memoryView->setSortingEnabled(false);
  layout->addWidget(memoryView, 0, 0, 1, 1);
  this->setWidget(dockContent);
  if(closable)
    window->memoryDockList.append(this);
  memoryView->setModel(window->memory);
  /*for(int i=0;i<2;i++)
    memoryView->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);*/
  memoryView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  memoryView->scrollTo(window->memory->index((startAddress/16)&0xFFFFFFF0));
  memoryView->horizontalHeader()->setFixedHeight(20);

  QHeaderView *verticalHeader = memoryView->verticalHeader();
  verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
  verticalHeader->setDefaultSectionSize(20);

  window->memoryDockList.append(this);
}

MemoryDockWidget::~MemoryDockWidget()
{
  window->memoryDockList.removeOne(this);
}

void MemoryDockWidget::scrollToAddress(Address address) {
  int i = (address/16)&0xFFFFFFF0;
  this->memoryView->scrollTo(window->memory->index(i), QAbstractItemView::PositionAtTop);
}
