// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QTextEdit>

#include "DolphinQt2/GameList/GameFile.h"

class InfoWidget final : public QWidget
{
Q_OBJECT
public:
	explicit InfoWidget(GameFile game);

private slots:
	void ComputeChecksum();
	void ChangeLanguage();
	void SaveBanner();

private:
	QGroupBox* CreateBannerDetails();
	QGroupBox* CreateISODetails();
	QLineEdit* CreateValueDisplay() { return CreateValueDisplay(QStringLiteral("")); };
	QLineEdit* CreateValueDisplay(QString value);
	QWidget* CreateChecksumComputer();
	void CreateLanguageSelector();
	QWidget* CreateBannerGraphic();

	GameFile m_game;
	QLineEdit* m_checksum_result;
	QComboBox* m_language_selector;
	QLineEdit* m_long_name;
	QLineEdit* m_short_name;
	QLineEdit* m_short_maker;
	QLineEdit* m_long_maker;
	QTextEdit* m_description;
};
