#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "lv2selectorwindow.h"
#include "soundengine.h"

#include "project.h"
#include "projectwindow.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::begin()
{
    SoundEngine::initialize();
    SoundEngine::openDefaultOutput();
}

static Lv2SelectorWindow *lv2Window = NULL;

void MainWindow::on_actionLv2PluginBrowser_triggered()
{
	if (!lv2Window)
		lv2Window = new Lv2SelectorWindow(this);
	lv2Window->hide();
	lv2Window->show();
}

void MainWindow::on_actionQuit_triggered()
{
    this->close();
}

void MainWindow::on_actionNewProject_triggered()
{
    GenesisProject *project = genesis_project_new();
    ProjectWindow *window = new ProjectWindow(project, this);
    window->show();
}
