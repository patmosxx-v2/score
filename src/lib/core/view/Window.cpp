// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <score/actions/Menu.hpp>
#include <score/plugins/application/GUIApplicationPlugin.hpp>
#include <score/plugins/documentdelegate/DocumentDelegateView.hpp>
#include <score/plugins/panel/PanelDelegate.hpp>
#include <score/widgets/MarginLess.hpp>

#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>
#include <core/document/DocumentView.hpp>
#include <core/presenter/Presenter.hpp>
#include <core/view/Window.hpp>

#include <ossia/detail/algorithms.hpp>

#include <QCloseEvent>
#include <QDockWidget>
#include <QEvent>
#include <QToolTip>
#include <QApplication>
#include <QLabel>
#include <QScreen>
#include <QToolButton>
#include <QSplitter>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QStyleOptionTitleBar>
#include <qcoreevent.h>
#include <qnamespace.h>

#include <wobjectimpl.h>

#include <QPainter>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolBar>
#include <set>
W_OBJECT_IMPL(score::View)
namespace score
{
struct PanelComparator
{
  bool operator()(
      const QPair<score::PanelDelegate*, QWidget*>& lhs,
      const QPair<score::PanelDelegate*, QWidget*>& rhs) const
  {
    return lhs.first->defaultPanelStatus().priority
           < rhs.first->defaultPanelStatus().priority;
  }
};
View::~View() {}

static void
setTitle(View& view, const score::Document* document, bool save_state) noexcept
{
  QString title;

  if (document)
  {
    if (save_state)
    {
      title = " * ";
    }
    title += QStringLiteral("score %1 - %2")
                 .arg(qApp->applicationVersion())
                 .arg(document->metadata().fileName());
  }
  else
  {
    title = QStringLiteral("score %1").arg(qApp->applicationVersion());
  }

  view.setWindowIconText(title);
  view.setWindowTitle(title);
}

W_OBJECT_IMPL(FixedTabWidget)
class RectSplitter : public QSplitter
{
public:
  QBrush brush ;
  using QSplitter::QSplitter;
  void paintEvent(QPaintEvent* ev) override
  {if(brush == QBrush()) return;
    QPainter p{this};
    p.setPen(Qt::transparent);
    p.setBrush(brush);
    p.drawRoundedRect(rect(), 3, 3);
  }
};
class RectWidget : public QWidget
{
public:
  void paintEvent(QPaintEvent* ev) override
  {
    QPainter p{this};
    p.setPen(Qt::transparent);
    p.setBrush(QColor("#1F1F20"));
    p.drawRect(rect());
  }
};
/*
class RectTabWidget: public QTabWidget
{
public:
  using QTabWidget::QTabWidget;
  void paintEvent(QPaintEvent* ev) override
  {
    QPainter p{this};
    p.setPen(palette().color(QPalette::Button));
    p.setBrush(QColor("#21FF25"));
    p.drawRoundedRect(rect(), 3, 3);
  }
};
*/
class BottomToolbarWidget : public QWidget
{
public:
  void paintEvent(QPaintEvent* ev) override
  {
    QPainter p{this};
    p.fillRect(rect(), Qt::transparent);

  }
};

class TitleBar : public QWidget
{
public:
  TitleBar()
  {
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
  }

  void setText(const QString& txt)
  {
    m_text = txt.toUpper();
    update();
  }

  QSize sizeHint() const override
  {
    return {100, 15};
  }

  void paintEvent(QPaintEvent* ev) override
  {
    QPainter painter{this};
    painter.setFont(QFont("Ubuntu", 9, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, m_text);
  }

private:
  QString m_text;
};

View::View(QObject* parent)
  : QMainWindow{}
{
  setAutoFillBackground(false);
  setObjectName("View");
  setWindowIcon(QIcon("://ossia-score.png"));
  setTitle(*this, nullptr, false);

  setIconSize(QSize{24,24});

  auto leftLabel = new TitleBar;
  leftTabs = new FixedTabWidget;
  connect(leftTabs, &FixedTabWidget::actionTriggered,
          this, [=] (QAction* act, bool b) {
    leftLabel->setText(act->text());
  });
  ((QVBoxLayout*)leftTabs->layout())->insertWidget(0, leftLabel);
  rightSplitter = new RectSplitter{Qt::Vertical};
  auto rect = QGuiApplication::primaryScreen()->availableGeometry();
  this->resize(
      static_cast<int>(rect.width() * 0.75),
      static_cast<int>(rect.height() * 0.75));

  auto totalWidg = new RectSplitter;
  totalWidg->setContentsMargins(0,0,0,0);
  totalWidg->addWidget(leftTabs);

  {
    auto rs = new RectSplitter{Qt::Vertical};
    rs->brush = QColor("#1F1F20");
    centralDocumentWidget = rs;
    totalWidg->addWidget(centralDocumentWidget);
    centralDocumentWidget->setContentsMargins(0, 0, 0, 0);

    //auto lay = new score::MarginLess<QVBoxLayout>(centralDocumentWidget);
    centralTabs = new QTabWidget;
    centralTabs->setContentsMargins(0, 0, 0, 0);
    centralTabs->setMovable(false);

    centralTabs->setObjectName("Documents");
    centralTabs->setContentsMargins(0, 0, 0, 0);
    centralTabs->tabBar()->setDocumentMode(true);
    centralTabs->tabBar()->setDrawBase(false);
    centralTabs->tabBar()->setAutoHide(true);
    //centralTabs->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    rs->addWidget(centralTabs);

    transportBar = new BottomToolbarWidget;
    auto transportLay = new score::MarginLess<QGridLayout>{transportBar};
    QPalette pal;
    pal.setColor(QPalette::Window, Qt::blue);
    transportBar->setPalette(pal);
    transportBar->setFixedHeight(35);
    rs->addWidget(transportBar);

    bottomTabs = new FixedTabWidget;
    bottomTabs->brush = qApp->palette().brush(QPalette::Window);
    bottomTabs->setContentsMargins(0, 0, 0, 0);
    bottomTabs->setMaximumHeight(300);
#if QT_VESION >= QT_VERSION_CHECK(5, 14, 0)
    bottomTabs->actionGroup()->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
#endif
    rs->addWidget(bottomTabs);
    rs->setCollapsible(0, false);
    rs->setCollapsible(1, false);
    rs->setCollapsible(2, true);
    QList<int> sz = rs->sizes();
    sz[2] = 0;
    rs->setSizes(sz);
    connect(bottomTabs, &FixedTabWidget::actionTriggered, this, [rs, leftLabel, this] (QAction* act, bool ok) {
      if(ok)
      {
        QList<int> sz = rs->sizes();
        if(sz[2] <= 1)
        {
          sz[2] = 200;
          rs->setSizes(sz);
        }
      }
      else
      {
        QList<int> sz = rs->sizes();
        sz[2] = 0;
        rs->setSizes(sz);
      }
    });
  }
  totalWidg->addWidget(rightSplitter);
  totalWidg->setHandleWidth(1);

  setCentralWidget(totalWidg);
  connect(
      centralTabs,
      &QTabWidget::currentChanged,
      this,
      [&](int index) {
        static QMetaObject::Connection saved_connection;
        QObject::disconnect(saved_connection);
        auto widg = centralTabs->widget(index);
        auto doc = m_documents.find(widg);
        if (doc == m_documents.end())
        {
          setTitle(*this, nullptr, false);
          return;
        }

        auto& document = const_cast<score::Document&>(doc->second->document());
        activeDocumentChanged(document.model().id());

        setTitle(*this, &document, !document.commandStack().isAtSavedIndex());
        saved_connection = connect(
            &document.commandStack(),
            &score::CommandStack::saveIndexChanged,
            this,
            [this, doc = &document](bool state) {
              setTitle(*this, doc, !state);
            });
      },
      Qt::QueuedConnection);

  connect(centralTabs, &QTabWidget::tabCloseRequested, this, [&](int index) {
    closeRequested(
        m_documents.at(centralTabs->widget(index))->document().model().id());
  });
}
void View::setPresenter(Presenter* p)
{
  m_presenter = p;
}
void View::addDocumentView(DocumentView* doc)
{
  doc->setParent(this);
  auto widg = doc->viewDelegate().getWidget();
  m_documents.insert(std::make_pair(widg, doc));
  centralTabs->addTab(widg, doc->document().metadata().fileName());
  centralTabs->setCurrentIndex(centralTabs->count() - 1);
  centralTabs->setTabsClosable(true);
  sizeChanged(size());
}

class HelperPanelDelegate : public PanelDelegate
{
public:
  HelperPanelDelegate(const score::GUIApplicationContext& ctx)
      : PanelDelegate{ctx}
  {
    widg = new QWidget;
    widg->setContentsMargins(3, 2, 3, 2);
    widg->setMinimumHeight(100);
    widg->setMaximumHeight(100);
    widg->setMinimumWidth(180);

    auto l = new QVBoxLayout{widg};

    status = new QLabel;
    status->setTextFormat(Qt::RichText);
    status->setText("<i>Remember those quiet evenings</i>");
    status->setWordWrap(true);

#ifndef QT_NO_STYLE_STYLESHEET
    status->setStyleSheet("color: #787876;");
#endif
    l->addWidget(status);
    l->addStretch(12);
  }

  QWidget* widget() override { return widg; }

  const PanelStatus& defaultPanelStatus() const override
  {
    static const PanelStatus stat{true,
                                  true,
                                  Qt::RightDockWidgetArea,
                                  -100000,
                                  "Info",
                                  "info",
                                  QKeySequence::HelpContents};
    return stat;
  }
  QWidget* widg{};
  QLabel* status{};
};
void View::setupPanel(PanelDelegate* v)
{
  {
    // First time we get there, register the additional helper panel
    static int ok = false;
    if (!ok)
    {
      ok = true;
      static HelperPanelDelegate hd(v->context());
      m_status = hd.status;
      setupPanel(&hd);
    }
  }
  using namespace std;
  auto w = v->widget();

  QAction* toggle{};
  // Add the panel
  switch (v->defaultPanelStatus().dock)
  {
    case Qt::LeftDockWidgetArea:
    {
      auto [idx, act] = leftTabs->addTab(w, v->defaultPanelStatus());
      toggle = act;

      break;
    }
    case Qt::RightDockWidgetArea:
    {
      rightSplitter->insertWidget(0, w);

      break;
    }
    case Qt::BottomDockWidgetArea:
    {
      auto [tabIdx, act] = bottomTabs->addTab(w, v->defaultPanelStatus());
      toggle = act;

      break;
    }
    default:
      SCORE_ABORT;
  }

  if(toggle)
  {
    auto& mw = v->context().menus.get().at(score::Menus::Windows());
    addAction(toggle);
    mw.menu()->addAction(toggle);

    // Maybe show the panel
    if(v->defaultPanelStatus().shown)
        toggle->toggle();
  }
}

void View::allPanelsAdded()
{
  for(auto& panel : score::GUIAppContext().panels())
  {
    if(panel.defaultPanelStatus().prettyName == QObject::tr("Inspector"))
    {
      auto splitter = (RectSplitter*)centralWidget();
      auto act = bottomTabs->addAction(rightSplitter->widget(1), panel.defaultPanelStatus());
      connect(act, &QAction::toggled, this, [splitter] (bool ok) {
        if(ok)
        {
          QList<int> sz = splitter->sizes();
          if(sz[2] <= 1)
          {
            sz[2] = 200;
            splitter->setSizes(sz);
          }
        }
        else
        {
          QList<int> sz = splitter->sizes();
          sz[2] = 0;
          splitter->setSizes(sz);
        }
      });
      break;
    }
  }

  // Show the device explorer first
  leftTabs->toolbar()->actions().front()->trigger();
}

void View::closeDocument(DocumentView* doc)
{
  for (int i = 0; i < centralTabs->count(); i++)
  {
    auto widg = doc->viewDelegate().getWidget();
    if (widg == centralTabs->widget(i))
    {
      m_documents.erase(widg);

      centralTabs->removeTab(i);
      return;
    }
  }
}

void View::restoreLayout()
{
}

void View::closeEvent(QCloseEvent* ev)
{
  if (m_presenter->exit())
  {
    ev->accept();
  }
  else
  {
    ev->ignore();
  }
}

void View::on_fileNameChanged(DocumentView* d, const QString& newName)
{
  for (int i = 0; i < centralTabs->count(); i++)
  {
    if (d->viewDelegate().getWidget() == centralTabs->widget(i))
    {
      QString n = newName;
      while (n.contains("/"))
      {
        n.remove(0, n.indexOf("/") + 1);
      }
      n.truncate(n.lastIndexOf("."));
      centralTabs->setTabText(i, n);
      return;
    }
  }
}

void View::changeEvent(QEvent* ev)
{
  if (m_presenter)
    if (ev->type() == QEvent::ActivationChange)
    {
      for (GUIApplicationPlugin* ctrl :
           m_presenter->applicationContext().guiApplicationPlugins())
      {
        ctrl->on_activeWindowChanged();
      }
    }

  QMainWindow::changeEvent(ev);
}

void View::resizeEvent(QResizeEvent* e)
{
  QMainWindow::resizeEvent(e);
  sizeChanged(e->size());
}
bool score::View::event(QEvent* event)
{
  if (event->type() == QEvent::StatusTip)
  {
    auto tip = ((QStatusTipEvent*)event)->tip();
    auto idx = tip.indexOf(QChar('\n'));
    if (idx != -1)
    {
      tip.insert(idx, "</b>\n");
      tip.push_front("<b>");
    }
    else
    {
      tip.push_front("<b>");
      tip.push_back("</b>");
    }
    tip.replace(QChar('\n'), "</br>");
    m_status->setText(tip);
  }
  else if (event->type() == QEvent::ToolTip)
  {
    auto tip = ((QHelpEvent*)event)->globalPos();
    auto pos = mapFromGlobal(tip);
    if(auto w = childAt(pos))
    {
      m_status->setText(w->statusTip());
    }
  }

  return QMainWindow::event(event);
}

FixedTabWidget::FixedTabWidget() noexcept
  : m_buttons{new QToolBar}
{
  this->setContentsMargins(2, 2, 2, 2);
  this->setLayout(&m_layout);
  auto layout = new QVBoxLayout;
  layout->setMargin(9);
  layout->setSpacing(6);
  layout->addWidget(&m_stack);
  m_layout.addLayout(layout);
  m_layout.addWidget(m_buttons);
  QPalette transp = this->palette();
  transp.setColor(QPalette::Background, Qt::transparent);
  m_buttons->setPalette(transp);
  m_buttons->setIconSize(QSize{24,24});
  m_buttons->setContentsMargins(0, 0, 0, 0);

  m_actGrp = new QActionGroup{m_buttons};
  m_actGrp->setExclusive(true);
}


QActionGroup* FixedTabWidget::actionGroup() const noexcept { return m_actGrp; }


QToolBar* FixedTabWidget::toolbar() const noexcept { return m_buttons; }


QSize FixedTabWidget::sizeHint() const
{
  return {200, 1000};
}


void FixedTabWidget::setTab(int index)
{
  if(m_actGrp->actions()[index]->isChecked())
    return;

  m_actGrp->actions()[index]->trigger();
}


std::pair<int, QAction*> FixedTabWidget::addTab(QWidget* widg, const PanelStatus& v)
{
  int idx = m_stack.addWidget(widg);

  auto btn = m_buttons->addAction(v.icon, v.prettyName);
  m_actGrp->addAction(btn);
  btn->setCheckable(true);
  btn->setShortcut(v.shortcut);
  btn->setIcon(v.icon);

  btn->setToolTip(v.prettyName);
  btn->setWhatsThis(widg->whatsThis());
  btn->setStatusTip(widg->statusTip());

  connect(btn, &QAction::triggered,
          &m_stack, [this, btn, idx] (bool checked) {
    m_stack.setCurrentIndex(idx);
    actionTriggered(btn, checked);
  });

  return std::make_pair(idx, btn);
}

QAction* FixedTabWidget::addAction(QWidget* widg, const PanelStatus& v)
{
  auto btn = m_buttons->addAction(v.icon, v.prettyName);
  m_actGrp->addAction(btn);
  btn->setCheckable(true);
  btn->setShortcut(v.shortcut);
  btn->setIcon(v.icon);

  btn->setToolTip(v.prettyName);
  btn->setWhatsThis(widg->whatsThis());
  btn->setStatusTip(widg->statusTip());

  return btn;
}


void FixedTabWidget::paintEvent(QPaintEvent* ev)
{
  if(brush == QBrush()) return;
  QPainter p{this};
  p.setPen(Qt::transparent);
  //p.setBrush(QColor("#121216"));
  p.setBrush(brush);
  p.drawRoundedRect(rect(), 3, 3);
}

}
