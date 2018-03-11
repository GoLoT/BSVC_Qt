#ifndef MEMORYDOCKWIDGET_H
#define MEMORYDOCKWIDGET_H

#include <QDockWidget>
#include <QTableView>
#include "Framework/Types.hpp"
#include "qtinterfacetypes.h"

class MainWindow;
namespace Ui {
  class MemoryDockWidget;
}

class MemoryDockWidget : public QDockWidget
{
  Q_OBJECT

public:
  explicit MemoryDockWidget(MainWindow* window, Address startAddress = 0, bool closable = true, QWidget *parent = 0);
  ~MemoryDockWidget();
  void scrollToAddress(Address address);

private:
  Ui::MemoryDockWidget *ui;
  QTableView* memoryView;
  MainWindow* window;
};

#endif // MEMORYDOCKWIDGET_H
