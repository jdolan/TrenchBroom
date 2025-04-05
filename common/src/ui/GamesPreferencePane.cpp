/*
 Copyright (C) 2010 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GamesPreferencePane.h"

#include <QAction>
#include <QBoxLayout>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolButton>
#include <QWidget>

#include "FileLogger.h"
#include "io/DiskIO.h"
#include "io/PathQt.h"
#include "io/ResourceUtils.h"
#include "mdl/GameConfig.h"
#include "mdl/GameFactory.h"
#include "ui/BorderLine.h"
#include "ui/FormWithSectionsLayout.h"
#include "ui/GameEngineDialog.h"
#include "ui/GameListBox.h"
#include "ui/MapDocument.h"
#include "ui/QtUtils.h"
#include "ui/ViewConstants.h"

namespace tb::ui
{

GamesPreferencePane::GamesPreferencePane(MapDocument* document, QWidget* parent)
  : PreferencePane{parent}
  , m_document{document}
{
  createGui();
  updateControls();
  m_gameListBox->setFocus();
}

void GamesPreferencePane::createGui()
{
  m_gameListBox = new GameListBox{};
  m_gameListBox->selectGame(0);
  m_gameListBox->setMaximumWidth(220);
  m_gameListBox->setMinimumHeight(300);

  m_defaultPage = createDefaultPage(tr("Select a game."));

  m_stackedWidget = new QStackedWidget{};
  m_stackedWidget->addWidget(m_defaultPage);

  auto* showUserConfigDirButton =
    createBitmapButton("Folder.svg", tr("Open custom game configurations folder"));
  connect(
    showUserConfigDirButton,
    &QAbstractButton::clicked,
    this,
    &GamesPreferencePane::showUserConfigDirClicked);

  auto* buttonLayout = createMiniToolBarLayoutRightAligned(showUserConfigDirButton);

  auto* glbLayout = new QVBoxLayout{};
  glbLayout->addWidget(m_gameListBox);
  glbLayout->addWidget(new BorderLine(BorderLine::Direction::Horizontal));
  glbLayout->addLayout(buttonLayout);

  auto* stwLayout = new QVBoxLayout{};
  stwLayout->setContentsMargins(
    LayoutConstants::DialogOuterMargin,
    LayoutConstants::DialogOuterMargin,
    LayoutConstants::DialogOuterMargin,
    LayoutConstants::DialogOuterMargin);
  stwLayout->setSpacing(LayoutConstants::WideVMargin);
  stwLayout->addWidget(m_stackedWidget, 1, Qt::AlignTop);

  auto* layout = new QHBoxLayout{};
  layout->setContentsMargins(QMargins{});
  layout->setSpacing(0);
  setLayout(layout);

  layout->addLayout(glbLayout);
  layout->addWidget(new BorderLine(BorderLine::Direction::Vertical));
  layout->addSpacing(LayoutConstants::MediumVMargin);
  layout->addLayout(stwLayout, 1);

  setMinimumWidth(600);

  connect(
    m_gameListBox, &GameListBox::currentGameChanged, this, [&]() { updateControls(); });
}

void GamesPreferencePane::showUserConfigDirClicked()
{
  auto& gameFactory = mdl::GameFactory::instance();
  auto path = gameFactory.userGameConfigsPath().lexically_normal();

  io::Disk::createDirectory(path) | kdl::transform([&](auto) {
    const auto url = QUrl::fromLocalFile(io::pathAsQPath(path));
    QDesktopServices::openUrl(url);
  }) | kdl::transform_error([&](auto e) {
    if (m_document)
    {
      m_document->error() << e.msg;
    }
    else
    {
      FileLogger::instance().error() << e.msg;
    }
  });
}

bool GamesPreferencePane::canResetToDefaults()
{
  return false;
}

void GamesPreferencePane::doResetToDefaults() {}

void GamesPreferencePane::updateControls()
{
  m_gameListBox->updateGameInfos();

  const auto desiredGame = m_gameListBox->selectedGameName();
  if (desiredGame.empty())
  {
    m_stackedWidget->setCurrentWidget(m_defaultPage);
  }
  else if (m_currentGamePage && m_currentGamePage->gameName() == desiredGame)
  {
    // refresh the current page
    m_currentGamePage->updateControls();
  }
  else
  {
    // build a new current page
    delete m_currentGamePage;
    m_currentGamePage = new GamePreferencePane{desiredGame};

    m_stackedWidget->addWidget(m_currentGamePage);
    m_stackedWidget->setCurrentWidget(m_currentGamePage);

    connect(
      m_currentGamePage,
      &GamePreferencePane::requestUpdate,
      this,
      &GamesPreferencePane::updateControls);
  }
}

bool GamesPreferencePane::validate()
{
  return true;
}

// GamePreferencePane

GamePreferencePane::GamePreferencePane(std::string gameName, QWidget* parent)
  : QWidget{parent}
  , m_gameName{std::move(gameName)}
{
  createGui();
}

void GamePreferencePane::createGui()
{
  m_gamePathText = new QLineEdit{};
  m_gamePathText->setPlaceholderText(tr("Click on the button to change..."));
  connect(m_gamePathText, &QLineEdit::editingFinished, this, [this]() {
    updateGamePath(m_gamePathText->text());
  });

  auto* validDirectoryIcon = new QAction{m_gamePathText};
  m_gamePathText->addAction(validDirectoryIcon, QLineEdit::TrailingPosition);
  connect(
    m_gamePathText,
    &QLineEdit::textChanged,
    this,
    [validDirectoryIcon](const QString& text) {
      if (text.isEmpty() || QDir{text}.exists())
      {
        validDirectoryIcon->setToolTip("");
        validDirectoryIcon->setIcon(QIcon{});
      }
      else
      {
        validDirectoryIcon->setToolTip(tr("Directory not found"));
        validDirectoryIcon->setIcon(io::loadSVGIcon("IssueBrowser.svg"));
      }
    });

  auto* chooseGamePathButton = new QPushButton{tr("...")};
  connect(
    chooseGamePathButton,
    &QPushButton::clicked,
    this,
    &GamePreferencePane::chooseGamePathClicked);

  auto* configureEnginesButton = new QPushButton{tr("Configure engines...")};
  connect(
    configureEnginesButton,
    &QPushButton::clicked,
    this,
    &GamePreferencePane::configureEnginesClicked);

  auto* gamePathLayout = new QHBoxLayout{};
  gamePathLayout->setContentsMargins(QMargins{});
  gamePathLayout->setSpacing(LayoutConstants::MediumHMargin);
  gamePathLayout->addWidget(m_gamePathText, 1);
  gamePathLayout->addWidget(chooseGamePathButton);

  auto* layout = new FormWithSectionsLayout{};
  layout->setContentsMargins(0, LayoutConstants::MediumVMargin, 0, 0);
  layout->setVerticalSpacing(2);
  layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

  layout->addSection(QString::fromStdString(m_gameName));
  layout->addRow(tr("Game Path"), gamePathLayout);
  layout->addRow("", configureEnginesButton);

  layout->addSection(tr("Compilation Tools"));

  auto& gameFactory = mdl::GameFactory::instance();
  const auto& gameConfig = gameFactory.gameConfig(m_gameName);

  for (auto& tool : gameConfig.compilationTools)
  {
    const auto toolName = tool.name;
    auto* edit = new QLineEdit{};
    edit->setText(
      io::pathAsQString(gameFactory.compilationToolPath(m_gameName, toolName)));
    if (tool.description)
    {
      edit->setToolTip(QString::fromStdString(*tool.description));
    }
    connect(edit, &QLineEdit::editingFinished, this, [this, toolName, edit]() {
      mdl::GameFactory::instance().setCompilationToolPath(
        m_gameName, toolName, io::pathFromQString(edit->text()));
    });

    auto* browseButton = new QPushButton{"..."};
    connect(browseButton, &QPushButton::clicked, this, [this, toolName, edit]() {
      const auto pathStr = QFileDialog::getOpenFileName(
        this,
        tr("%1 Path").arg(QString::fromStdString(toolName)),
        fileDialogDefaultDirectory(FileDialogDir::CompileTool));
      if (!pathStr.isEmpty())
      {
        edit->setText(pathStr);
        if (mdl::GameFactory::instance().setCompilationToolPath(
              m_gameName, toolName, io::pathFromQString(pathStr)))
        {
          emit requestUpdate();
        }
      }
    });

    auto* rowLayout = new QHBoxLayout{};
    rowLayout->setContentsMargins(QMargins{});
    rowLayout->setSpacing(LayoutConstants::MediumHMargin);
    rowLayout->addWidget(edit, 1);
    rowLayout->addWidget(browseButton);

    layout->addRow(QString::fromStdString(tool.name), rowLayout);
  }

  setLayout(layout);

  updateControls();
}

void GamePreferencePane::chooseGamePathClicked()
{
  const auto pathStr = QFileDialog::getExistingDirectory(
    this, tr("Game Path"), fileDialogDefaultDirectory(FileDialogDir::GamePath));
  if (!pathStr.isEmpty())
  {
    updateGamePath(pathStr);
  }
}

void GamePreferencePane::updateGamePath(const QString& str)
{
  updateFileDialogDefaultDirectoryWithDirectory(FileDialogDir::GamePath, str);

  const auto gamePath = io::pathFromQString(str);
  auto& gameFactory = mdl::GameFactory::instance();
  if (gameFactory.setGamePath(m_gameName, gamePath))
  {
    emit requestUpdate();
  }
}

void GamePreferencePane::configureEnginesClicked()
{
  auto dialog = GameEngineDialog{m_gameName, this};
  dialog.exec();
}

const std::string& GamePreferencePane::gameName() const
{
  return m_gameName;
}

void GamePreferencePane::updateControls()
{
  auto& gameFactory = mdl::GameFactory::instance();

  // Refresh tool paths from preferences
  for (const auto& [toolName, toolPathEditor] : m_toolPathEditors)
  {
    toolPathEditor->setText(
      io::pathAsQString(gameFactory.compilationToolPath(m_gameName, toolName)));
  }

  // Refresh game path
  const auto gamePath = gameFactory.gamePath(m_gameName);
  m_gamePathText->setText(io::pathAsQString(gamePath));
}

} // namespace tb::ui
