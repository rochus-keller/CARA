/*
 * Copyright 2000-2015 Rochus Keller <mailto:rkeller@nmr.ch>
 *
 * This file is part of CARA (Computer Aided Resonance Assignment,
 * see <http://cara.nmr.ch/>).
 *
 * CARA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation, either version 2 of
 * the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "FourDScopeAgent.h"
#include <qtextstream.h> 
#include <qinputdialog.h> 
#include <qmessagebox.h>
#include <qcolordialog.h>
#include <Root/Any.h>
#include <Root/MakroCommand.h>
#include <Lexi/MainWindow.h>
#include <Lexi/Background.h>
#include <Lexi/Label.h>
#include <Lexi/ContextMenu.h>
#include <Spec/SpecProjector.h>
#include <Spec/SpectrumType.h>
#include <Spec/SpectrumPeer.h>
#include <Spec/Repository.h>
#include <Spec/SpecRotator.h>
#include <Spec/SpinPointSpace.h>
#include <Spec/PeakListPeer.h>
#include <SpecView/CursorMdl.h>
#include <SpecView/ViewAreaMdl.h>
#include <SpecView/SpecViewer.h>
#include <SpecView/FocusCtrl.h>
#include <SpecView/ContourView.h>
#include <SpecView/CursorView.h>
#include <SpecView/CursorCtrl.h>
#include <SpecView/ScrollCtrl.h>
#include <SpecView/ZoomCtrl.h>
#include <SpecView/SelectZoomCtrl.h>
#include <SpecView/SliceView.h>
#include <SpecView/CenterLine.h>
#include <SpecView/SelectRulerCtr.h>
#include <SpecView/FoldingView.h>
#include <Spec/GlobalCursor.h>
#include <SpecView/PointSelectCtrl.h>
#include <SpecView/PeakSelectCtrl.h>
#include <SingleAlignmentView.h>
#include <SpecView/RotateDlg.h>
#include <Dlg.h>
#include <SpecView/GotoDlg.h>
#include <ReportViewer.h>
#include <SpecView/DynValueEditor.h>
#include <SpecView/CandidateDlg.h>
#include <SpecView/SpecBatchList.h>
#include "ProposeSpinDlg.h"
#include <SpecView/PathSimDlg.h>
#include <QtDebug>
using namespace Spec;

GCC_IGNORE(-Wparentheses);

static const int BACKGROUND = 0;
static const int INTENSITY = 1;
static const int CONTOUR = 2;
static const int FOLDING = 3;
static const int LABEL1 = 4;
static const int LABEL2 = 5;
static const int LABEL3 = 6;
static const int RULER1 = 7;
static const int RULER2 = 8;
static const int CURSORS = 9;
static const int TUPLES = 10;
static const int PEAKS = 11;
static const int VIEWCOUNT = 12;

static const bool s_showContour = true;
static const float s_contourFactor = 1.4f;
static const ContourView::Option s_contourOption = ContourView::Both;
static const float s_gain = 2.0;

static QColor g_clrLabel = Qt::yellow;
static QColor g_clrSlice4D = Qt::cyan;
static QColor g_clrPeak = Qt::yellow;

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr FourDScopeAgent::SetResolution = "SetResolution";
Root::Action::CmdStr FourDScopeAgent::ShowLowRes = "ShowLowRes";
Root::Action::CmdStr FourDScopeAgent::Forward = "Forward";
Root::Action::CmdStr FourDScopeAgent::Backward = "Backward";
Root::Action::CmdStr FourDScopeAgent::FitWindow = "FitWindow";
Root::Action::CmdStr FourDScopeAgent::ShowFolded = "ShowFolded";
Root::Action::CmdStr FourDScopeAgent::SpecCalibrate = "SpecCalibrate";
Root::Action::CmdStr FourDScopeAgent::AutoCenter = "AutoCenter";
Root::Action::CmdStr FourDScopeAgent::ShowContour = "ShowContour";
Root::Action::CmdStr FourDScopeAgent::ShowIntensity = "ShowIntensity";
Root::Action::CmdStr FourDScopeAgent::AutoContour = "AutoContour";
Root::Action::CmdStr FourDScopeAgent::ContourParams = "ContourParams";
Root::Action::CmdStr FourDScopeAgent::PickSystem = "PickSystem";
Root::Action::CmdStr FourDScopeAgent::PickHori = "PickHori";
Root::Action::CmdStr FourDScopeAgent::PickHori4D = "PickHori4D";
Root::Action::CmdStr FourDScopeAgent::PickVerti4D = "PickVerti4D";
Root::Action::CmdStr FourDScopeAgent::PickVerti = "PickVerti";
Root::Action::CmdStr FourDScopeAgent::MovePeak = "MovePeak";
Root::Action::CmdStr FourDScopeAgent::MovePeakAlias = "MovePeakAlias";
Root::Action::CmdStr FourDScopeAgent::LabelPeak = "LabelPeak";
Root::Action::CmdStr FourDScopeAgent::DeletePeak = "DeletePeak";
Root::Action::CmdStr FourDScopeAgent::DeleteSpinX = "DeleteSpinX";
Root::Action::CmdStr FourDScopeAgent::DeleteSpinY = "DeleteSpinY";
Root::Action::CmdStr FourDScopeAgent::DeleteSpinX4D = "DeleteSpinX4D";
Root::Action::CmdStr FourDScopeAgent::DeleteSpinY4D = "DeleteSpinY4D";
Root::Action::CmdStr FourDScopeAgent::HidePeak = "HidePeak";
Root::Action::CmdStr FourDScopeAgent::ShowAllPeaks = "ShowAllPeaks";
Root::Action::CmdStr FourDScopeAgent::ShowAlignment = "ShowAlignment";
Root::Action::CmdStr FourDScopeAgent::Assign = "Assign";
Root::Action::CmdStr FourDScopeAgent::Unassign = "Unassign";
Root::Action::CmdStr FourDScopeAgent::SetSystemType = "SetSystemType";
Root::Action::CmdStr FourDScopeAgent::ViewLabels = "ViewLabels";
Root::Action::CmdStr FourDScopeAgent::SelectSpec2D = "SelectSpec2D";
Root::Action::CmdStr FourDScopeAgent::LinkSystems = "LinkSystems";
Root::Action::CmdStr FourDScopeAgent::UnlinkPred = "UnlinkPred";
Root::Action::CmdStr FourDScopeAgent::UnlinkSucc = "UnlinkSucc";
Root::Action::CmdStr FourDScopeAgent::SelectSystems = "SelectSystems";
Root::Action::CmdStr FourDScopeAgent::SelectSpins = "SelectSpins";
Root::Action::CmdStr FourDScopeAgent::HoldReference = "HoldReference";
Root::Action::CmdStr FourDScopeAgent::UnhidePeak = "UnhidePeak";
Root::Action::CmdStr FourDScopeAgent::CreateReport = "CreateReport";
Root::Action::CmdStr FourDScopeAgent::AddRulerVerti = "AddRulerVerti";
Root::Action::CmdStr FourDScopeAgent::AddRulerHori = "AddRulerHori";
Root::Action::CmdStr FourDScopeAgent::RemoveRulers = "RemoveRulers";
Root::Action::CmdStr FourDScopeAgent::RemoveAllRulers = "RemoveAllRulers";
Root::Action::CmdStr FourDScopeAgent::AutoRuler = "AutoRuler";
Root::Action::CmdStr FourDScopeAgent::ProposeHori = "ProposeHori";
Root::Action::CmdStr FourDScopeAgent::ProposeVerti = "ProposeVerti";
Root::Action::CmdStr FourDScopeAgent::ProposePeak = "ProposePeak";
Root::Action::CmdStr FourDScopeAgent::SelectSpec4D = "SelectSpec4D";
Root::Action::CmdStr FourDScopeAgent::PickSpin4D = "PickSpin4D";
Root::Action::CmdStr FourDScopeAgent::MoveSpin4D = "MoveSpin4D";
Root::Action::CmdStr FourDScopeAgent::MoveSpinAlias4D = "MoveSpinAlias4D";
Root::Action::CmdStr FourDScopeAgent::DeleteSpins4D = "DeleteSpins4D";
Root::Action::CmdStr FourDScopeAgent::LabelSpin4D = "LabelSpin4D";
Root::Action::CmdStr FourDScopeAgent::SetWidth = "SetWidth";
Root::Action::CmdStr FourDScopeAgent::FitWindow4D = "FitWindow4D";
Root::Action::CmdStr FourDScopeAgent::AutoContour2 = "AutoContour2";
Root::Action::CmdStr FourDScopeAgent::ContourParams2 = "ContourParams2";
Root::Action::CmdStr FourDScopeAgent::ShowWithOff = "ShowWithOff";
Root::Action::CmdStr FourDScopeAgent::Show4dPlane = "Show3dPlane";
Root::Action::CmdStr FourDScopeAgent::AutoHide = "AutoHide";
Root::Action::CmdStr FourDScopeAgent::StripCalibrate = "StripCalibrate";
Root::Action::CmdStr FourDScopeAgent::ProposeSpin = "ProposeSpin";
Root::Action::CmdStr FourDScopeAgent::EditAttsSpinH = "EditAttsSpinH";
Root::Action::CmdStr FourDScopeAgent::EditAttsSpinV = "EditAttsSpinV";
Root::Action::CmdStr FourDScopeAgent::EditAttsLink = "EditAttsLink";
Root::Action::CmdStr FourDScopeAgent::EditAttsSysH = "EditAttsSysH";
Root::Action::CmdStr FourDScopeAgent::EditAttsSysV = "EditAttsSysV";
Root::Action::CmdStr FourDScopeAgent::EditAttsSpinX4D = "EditAttsSpinX4D";
Root::Action::CmdStr FourDScopeAgent::EditAttsSpinY4D = "EditAttsSpinY4D";
Root::Action::CmdStr FourDScopeAgent::CursorSync = "CursorSync";
Root::Action::CmdStr FourDScopeAgent::GotoSystem = "GotoSystem";
Root::Action::CmdStr FourDScopeAgent::NextSpec4D = "NextSpec4D";
Root::Action::CmdStr FourDScopeAgent::PrevSpec4D = "PrevSpec4D";
Root::Action::CmdStr FourDScopeAgent::NextSpec2D = "NextSpec2D";
Root::Action::CmdStr FourDScopeAgent::PrevSpec2D = "PrevSpec2D";
Root::Action::CmdStr FourDScopeAgent::ShowWithOff2 = "ShowWithOff2";
Root::Action::CmdStr FourDScopeAgent::ShowLinks = "ShowLinks";
Root::Action::CmdStr FourDScopeAgent::DeleteLinks = "DeleteLinks";
Root::Action::CmdStr FourDScopeAgent::LabelVerti = "LabelVerti";
Root::Action::CmdStr FourDScopeAgent::LabelHori = "LabelHori";
Root::Action::CmdStr FourDScopeAgent::SetCandidates = "SetCandidates";
Root::Action::CmdStr FourDScopeAgent::ShowInfered = "ShowInfered";
Root::Action::CmdStr FourDScopeAgent::ShowUnlabeled = "ShowUnlabeled";
Root::Action::CmdStr FourDScopeAgent::CreateLinks = "CreateLinks";
Root::Action::CmdStr FourDScopeAgent::ForceCross = "ForceCross";
Root::Action::CmdStr FourDScopeAgent::DeleteLinks4D = "DeleteLinks4D";
Root::Action::CmdStr FourDScopeAgent::ViewLabels4D = "ViewLabels4D";
Root::Action::CmdStr FourDScopeAgent::AutoGain = "AutoGain";
Root::Action::CmdStr FourDScopeAgent::AutoGain4D = "AutoGain4D";
Root::Action::CmdStr FourDScopeAgent::ShowGhosts = "ShowGhosts";
Root::Action::CmdStr FourDScopeAgent::AutoHold = "AutoHold";
Root::Action::CmdStr FourDScopeAgent::PickLabel4D = "PickLabel4D";
Root::Action::CmdStr FourDScopeAgent::ShowLinks2 = "ShowLinks2";
Root::Action::CmdStr FourDScopeAgent::ShowInfered2 = "ShowInfered2";
Root::Action::CmdStr FourDScopeAgent::ShowUnlabeled2 = "ShowUnlabeled2";
Root::Action::CmdStr FourDScopeAgent::GhostLabels = "GhostLabels";
Root::Action::CmdStr FourDScopeAgent::HidePeak4D = "HidePeak2";
Root::Action::CmdStr FourDScopeAgent::GotoPeak = "GotoPeak";
Root::Action::CmdStr FourDScopeAgent::RangeSync = "RangeSync";
Root::Action::CmdStr FourDScopeAgent::EditAttsSys4D = "EditAttsSys4D";
Root::Action::CmdStr FourDScopeAgent::EditAttsLink4D = "EditAttsLink4D";
Root::Action::CmdStr FourDScopeAgent::OverlayCount = "OverlayCount";
Root::Action::CmdStr FourDScopeAgent::ActiveOverlay = "ActiveOverlay";
Root::Action::CmdStr FourDScopeAgent::SetPosColor = "SetPosColor";
Root::Action::CmdStr FourDScopeAgent::SetNegColor = "SetNegColor";
Root::Action::CmdStr FourDScopeAgent::OverlaySpec = "OverlaySpec";
Root::Action::CmdStr FourDScopeAgent::CntFactor = "CntFactor";
Root::Action::CmdStr FourDScopeAgent::CntThreshold = "CntThreshold";
Root::Action::CmdStr FourDScopeAgent::CntOption = "CntOption";
Root::Action::CmdStr FourDScopeAgent::AddLayer = "AddLayer";
Root::Action::CmdStr FourDScopeAgent::ComposeLayers = "ComposeLayers";
Root::Action::CmdStr FourDScopeAgent::UseLinkColors = "UseLinkColors";
Root::Action::CmdStr FourDScopeAgent::UseLinkColors4D = "UseLinkColors4D";
Root::Action::CmdStr FourDScopeAgent::SetLinkParams = "SetLinkParams";
Root::Action::CmdStr FourDScopeAgent::SetLinkParams4D = "SetLinkParams4D";
Root::Action::CmdStr FourDScopeAgent::GotoPoint = "GotoPoint";
Root::Action::CmdStr FourDScopeAgent::NewPeakList = "NewPeakList";
Root::Action::CmdStr FourDScopeAgent::OpenPeakList = "OpenPeakList";
Root::Action::CmdStr FourDScopeAgent::SavePeakList = "SavePeakList";
Root::Action::CmdStr FourDScopeAgent::MapPeakList = "MapPeakList";
Root::Action::CmdStr FourDScopeAgent::PickPlPeak = "PickPlPeak";
Root::Action::CmdStr FourDScopeAgent::MovePlPeak = "MovePlPeak";
Root::Action::CmdStr FourDScopeAgent::MovePlAlias = "MovePlAlias";
Root::Action::CmdStr FourDScopeAgent::LabelPlPeak = "LabelPlPeak";
Root::Action::CmdStr FourDScopeAgent::DeletePlPeaks = "DeletePlPeaks";
Root::Action::CmdStr FourDScopeAgent::EditPlPeakAtts = "EditPlPeakAtts";
Root::Action::CmdStr FourDScopeAgent::SetPlColor = "SetPlColor";
Root::Action::CmdStr FourDScopeAgent::DeleteAliasPeak = "DeleteAliasPeak";
Root::Action::CmdStr FourDScopeAgent::FitWindowX = "FitWindowX";
Root::Action::CmdStr FourDScopeAgent::FitWindowY = "FitWindowY";
Root::Action::CmdStr FourDScopeAgent::GotoPlPeak = "GotoPlPeak";
Root::Action::CmdStr FourDScopeAgent::ViewPlLabels = "ViewPlLabels";
Root::Action::CmdStr FourDScopeAgent::SyncDepth = "SyncDepth";
Root::Action::CmdStr FourDScopeAgent::AdjustIntensity = "AdjustIntensity";
Root::Action::CmdStr FourDScopeAgent::ShowUnknown = "ShowUnknown";
Root::Action::CmdStr FourDScopeAgent::ShowUnknown2 = "ShowUnknown2";
Root::Action::CmdStr FourDScopeAgent::ShowPathSim = "ShowPathSim";

ACTION_SLOTS_BEGIN( FourDScopeAgent )
    { FourDScopeAgent::ShowPathSim, &FourDScopeAgent::handleShowPathSim },
    { FourDScopeAgent::ShowUnknown, &FourDScopeAgent::handleShowUnknown },
    { FourDScopeAgent::ShowUnknown2, &FourDScopeAgent::handleShowUnknown2 },
    { FourDScopeAgent::AdjustIntensity, &FourDScopeAgent::handleAdjustIntensity },
    { FourDScopeAgent::SyncDepth, &FourDScopeAgent::handleSyncDepth },
    { FourDScopeAgent::ViewPlLabels, &FourDScopeAgent::handleViewPlLabels },
    { FourDScopeAgent::GotoPlPeak, &FourDScopeAgent::handleGotoPlPeak },
    { FourDScopeAgent::FitWindowX, &FourDScopeAgent::handleFitWindowX },
    { FourDScopeAgent::FitWindowY, &FourDScopeAgent::handleFitWindowY },
    { FourDScopeAgent::DeleteAliasPeak, &FourDScopeAgent::handleDeleteAliasPeak },
    { FourDScopeAgent::MovePlAlias, &FourDScopeAgent::handleMovePlAlias },
    { FourDScopeAgent::SetPlColor, &FourDScopeAgent::handleSetPlColor },
    { FourDScopeAgent::NewPeakList, &FourDScopeAgent::handleNewPeakList },
    { FourDScopeAgent::OpenPeakList, &FourDScopeAgent::handleOpenPeakList },
    { FourDScopeAgent::SavePeakList, &FourDScopeAgent::handleSavePeakList },
    { FourDScopeAgent::MapPeakList, &FourDScopeAgent::handleMapPeakList },
    { FourDScopeAgent::PickPlPeak, &FourDScopeAgent::handlePickPlPeak },
    { FourDScopeAgent::MovePlPeak, &FourDScopeAgent::handleMovePlPeak },
    { FourDScopeAgent::LabelPlPeak, &FourDScopeAgent::handleLabelPlPeak },
    { FourDScopeAgent::DeletePlPeaks, &FourDScopeAgent::handleDeletePlPeaks },
    { FourDScopeAgent::EditPlPeakAtts, &FourDScopeAgent::handleEditPlPeakAtts },
    { FourDScopeAgent::GotoPoint, &FourDScopeAgent::handleGotoPoint },
    { FourDScopeAgent::SetLinkParams, &FourDScopeAgent::handleSetLinkParams },
    { FourDScopeAgent::SetLinkParams4D, &FourDScopeAgent::handleSetLinkParams4D },
    { FourDScopeAgent::UseLinkColors, &FourDScopeAgent::handleUseLinkColors },
    { FourDScopeAgent::UseLinkColors4D, &FourDScopeAgent::handleUseLinkColors4D },
    { FourDScopeAgent::ComposeLayers, &FourDScopeAgent::handleComposeLayers },
    { FourDScopeAgent::AddLayer, &FourDScopeAgent::handleAddLayer },
    { FourDScopeAgent::CntFactor, &FourDScopeAgent::handleCntFactor },
    { FourDScopeAgent::CntThreshold, &FourDScopeAgent::handleCntThreshold },
    { FourDScopeAgent::CntOption, &FourDScopeAgent::handleCntOption },
    { FourDScopeAgent::OverlaySpec, &FourDScopeAgent::handleOverlaySpec },
    { FourDScopeAgent::OverlayCount, &FourDScopeAgent::handleOverlayCount },
    { FourDScopeAgent::ActiveOverlay, &FourDScopeAgent::handleActiveOverlay },
    { FourDScopeAgent::SetPosColor, &FourDScopeAgent::handleSetPosColor },
    { FourDScopeAgent::SetNegColor, &FourDScopeAgent::handleSetNegColor },
    { FourDScopeAgent::EditAttsSys4D, &FourDScopeAgent::handleEditAttsSys4D },
    { FourDScopeAgent::EditAttsLink4D, &FourDScopeAgent::handleEditAttsLink4D },
    { FourDScopeAgent::RangeSync, &FourDScopeAgent::handleRangeSync },
    { FourDScopeAgent::GotoPeak, &FourDScopeAgent::handleGotoPeak },
    { FourDScopeAgent::HidePeak4D, &FourDScopeAgent::handleHidePeak4D },
    { FourDScopeAgent::GhostLabels, &FourDScopeAgent::handleGhostLabels },
    { FourDScopeAgent::ShowLinks2, &FourDScopeAgent::handleShowLinks2 },
    { FourDScopeAgent::ShowInfered2, &FourDScopeAgent::handleShowInfered2 },
    { FourDScopeAgent::PickLabel4D, &FourDScopeAgent::handlePickLabel4D },
    { FourDScopeAgent::AutoHold, &FourDScopeAgent::handleAutoHold },
    { FourDScopeAgent::ShowGhosts, &FourDScopeAgent::handleShowGhosts },
    { FourDScopeAgent::AutoGain4D, &FourDScopeAgent::handleAutoGain4D },
    { FourDScopeAgent::AutoGain, &FourDScopeAgent::handleAutoGain },
    { FourDScopeAgent::ViewLabels4D, &FourDScopeAgent::handleViewLabels4D },
    { FourDScopeAgent::DeleteLinks4D, &FourDScopeAgent::handleDeleteLinks4D },
    { FourDScopeAgent::ForceCross, &FourDScopeAgent::handleForceCross },
    { FourDScopeAgent::ShowUnlabeled2, &FourDScopeAgent::handleShowUnlabeled2 },
    { FourDScopeAgent::ShowUnlabeled, &FourDScopeAgent::handleShowUnlabeled },
    { FourDScopeAgent::CreateLinks, &FourDScopeAgent::handleCreateLinks },
    { FourDScopeAgent::ShowInfered, &FourDScopeAgent::handleShowInfered },
    { FourDScopeAgent::SetCandidates, &FourDScopeAgent::handleSetCandidates },
    { FourDScopeAgent::LabelVerti, &FourDScopeAgent::handleLabelVerti },
    { FourDScopeAgent::LabelHori, &FourDScopeAgent::handleLabelHori },
    { FourDScopeAgent::DeleteLinks, &FourDScopeAgent::handleDeleteLinks },
    { FourDScopeAgent::ShowLinks, &FourDScopeAgent::handleShowLinks },
    { FourDScopeAgent::ShowWithOff2, &FourDScopeAgent::handleShowWithOff2 },
    { FourDScopeAgent::NextSpec4D, &FourDScopeAgent::handleNextSpec4D },
    { FourDScopeAgent::PrevSpec4D, &FourDScopeAgent::handlePrevSpec4D },
    { FourDScopeAgent::NextSpec2D, &FourDScopeAgent::handleNextSpec2D },
    { FourDScopeAgent::PrevSpec2D, &FourDScopeAgent::handlePrevSpec2D },
    { FourDScopeAgent::GotoSystem, &FourDScopeAgent::handleGotoSystem },
    { FourDScopeAgent::CursorSync, &FourDScopeAgent::handleCursorSync },
    { FourDScopeAgent::EditAttsSpinH, &FourDScopeAgent::handleEditAttsSpinH },
    { FourDScopeAgent::EditAttsSpinV, &FourDScopeAgent::handleEditAttsSpinV },
    { FourDScopeAgent::EditAttsLink, &FourDScopeAgent::handleEditAttsLink },
    { FourDScopeAgent::EditAttsSysH, &FourDScopeAgent::handleEditAttsSysH },
    { FourDScopeAgent::EditAttsSysV, &FourDScopeAgent::handleEditAttsSysV },
    { FourDScopeAgent::EditAttsSpinY4D, &FourDScopeAgent::handleEditAttsSpinY4D },
    { FourDScopeAgent::EditAttsSpinX4D, &FourDScopeAgent::handleEditAttsSpinX4D },
    { FourDScopeAgent::ProposeSpin, &FourDScopeAgent::handleProposeSpin },
    { FourDScopeAgent::Show4dPlane, &FourDScopeAgent::handleShow4dPlane },
    { FourDScopeAgent::AutoHide, &FourDScopeAgent::handleAutoHide },
    { FourDScopeAgent::StripCalibrate, &FourDScopeAgent::handleStripCalibrate },
    { FourDScopeAgent::ShowWithOff, &FourDScopeAgent::handleShowWithOff },
    { FourDScopeAgent::AutoContour2, &FourDScopeAgent::handleAutoContour2 },
    { FourDScopeAgent::ContourParams2, &FourDScopeAgent::handleContourParams2 },
    { FourDScopeAgent::SetWidth, &FourDScopeAgent::handleSetWidth },
    { FourDScopeAgent::FitWindow4D, &FourDScopeAgent::handleFitWindow4D },
    { FourDScopeAgent::PickSpin4D, &FourDScopeAgent::handlePickSpin4D },
    { FourDScopeAgent::MoveSpin4D, &FourDScopeAgent::handleMoveSpin4D },
    { FourDScopeAgent::MoveSpinAlias4D, &FourDScopeAgent::handleMoveSpinAlias4D },
    { FourDScopeAgent::DeleteSpins4D, &FourDScopeAgent::handleDeleteSpins4D },
    { FourDScopeAgent::LabelSpin4D, &FourDScopeAgent::handleLabelSpin4D },
    { FourDScopeAgent::SelectSpec4D, &FourDScopeAgent::handleSelectSpec4D },
    { FourDScopeAgent::ProposeHori, &FourDScopeAgent::handleProposeHori },
    { FourDScopeAgent::ProposeVerti, &FourDScopeAgent::handleProposeVerti },
    { FourDScopeAgent::ProposePeak, &FourDScopeAgent::handleProposePeak },
    { FourDScopeAgent::AutoRuler, &FourDScopeAgent::handleAutoRuler },
    { FourDScopeAgent::AddRulerVerti, &FourDScopeAgent::handleAddRulerVerti },
    { FourDScopeAgent::AddRulerHori, &FourDScopeAgent::handleAddRulerHori },
    { FourDScopeAgent::RemoveRulers, &FourDScopeAgent::handleRemoveRulers },
    { FourDScopeAgent::RemoveAllRulers, &FourDScopeAgent::handleRemoveAllRulers },
    { FourDScopeAgent::CreateReport, &FourDScopeAgent::handleCreateReport },
    { FourDScopeAgent::UnhidePeak, &FourDScopeAgent::handleUnhidePeak },
    { FourDScopeAgent::HoldReference, &FourDScopeAgent::handleHoldReference },
    { FourDScopeAgent::ShowAllPeaks, &FourDScopeAgent::handleShowAllPeaks },
    { FourDScopeAgent::ShowAlignment, &FourDScopeAgent::handleShowAlignment },
    { FourDScopeAgent::Assign, &FourDScopeAgent::handleAssign },
    { FourDScopeAgent::Unassign, &FourDScopeAgent::handleUnassign },
    { FourDScopeAgent::SetSystemType, &FourDScopeAgent::handleSetSystemType },
    { FourDScopeAgent::ViewLabels, &FourDScopeAgent::handleViewLabels },
    { FourDScopeAgent::SelectSpec2D, &FourDScopeAgent::handleSelectSpec2D },
    { FourDScopeAgent::LinkSystems, &FourDScopeAgent::handleLinkSystems },
    { FourDScopeAgent::UnlinkPred, &FourDScopeAgent::handleUnlinkPred },
    { FourDScopeAgent::UnlinkSucc, &FourDScopeAgent::handleUnlinkSucc },
    { FourDScopeAgent::DeletePeak, &FourDScopeAgent::handleDeletePeak },
    { FourDScopeAgent::DeleteSpinX, &FourDScopeAgent::handleDeleteSpinX },
    { FourDScopeAgent::DeleteSpinY, &FourDScopeAgent::handleDeleteSpinY },
    { FourDScopeAgent::DeleteSpinX4D, &FourDScopeAgent::handleDeleteSpinX4D },
    { FourDScopeAgent::DeleteSpinY4D, &FourDScopeAgent::handleDeleteSpinY4D },
    { FourDScopeAgent::LabelPeak, &FourDScopeAgent::handleLabelPeak },
    { FourDScopeAgent::HidePeak, &FourDScopeAgent::handleHidePeak },
    { FourDScopeAgent::MovePeakAlias, &FourDScopeAgent::handleMovePeakAlias },
    { FourDScopeAgent::MovePeak, &FourDScopeAgent::handleMovePeak },
    { FourDScopeAgent::PickVerti, &FourDScopeAgent::handlePickVerti },
    { FourDScopeAgent::PickHori, &FourDScopeAgent::handlePickHori },
    { FourDScopeAgent::PickHori4D, &FourDScopeAgent::handlePickHori4D },
    { FourDScopeAgent::PickVerti4D, &FourDScopeAgent::handlePickVerti4D },
    { FourDScopeAgent::PickSystem, &FourDScopeAgent::handlePickSystem },
    { FourDScopeAgent::ContourParams, &FourDScopeAgent::handleContourParams },
    { FourDScopeAgent::AutoContour, &FourDScopeAgent::handleAutoContour },
    { FourDScopeAgent::ShowIntensity, &FourDScopeAgent::handleShowIntensity },
    { FourDScopeAgent::ShowContour, &FourDScopeAgent::handleShowContour },
    { FourDScopeAgent::AutoCenter, &FourDScopeAgent::handleAutoCenter },
    { FourDScopeAgent::SpecCalibrate, &FourDScopeAgent::handleSpecCalibrate },
    { FourDScopeAgent::ShowFolded, &FourDScopeAgent::handleShowFolded },
    { FourDScopeAgent::FitWindow, &FourDScopeAgent::handleFitWindow },
    { FourDScopeAgent::Backward, &FourDScopeAgent::handleBackward },
    { FourDScopeAgent::Forward, &FourDScopeAgent::handleForward },
    { FourDScopeAgent::ShowLowRes, &FourDScopeAgent::handleShowLowRes },
    { FourDScopeAgent::SetResolution, &FourDScopeAgent::handleSetResolution },
ACTION_SLOTS_END( FourDScopeAgent )

//////////////////////////////////////////////////////////////////////

#define MW ((Lexi::MainWindow*) getParent())->getQt()
static const Dimension s_defDim = 2;

FourDScopeAgent::FourDScopeAgent(Root::Agent* parent, Spectrum* spec, Project* pro):
	Root::Agent( parent ), d_lock( false ), d_autoRuler( false ), d_aol( 0 ),
		d_popSpec2D( 0 ), d_pl(0),
		d_popSpec4D( 0 ), d_popOrtho(0), d_popPlane(0)
{
	assert( spec && ( spec->getDimCount() == 2 || spec->getDimCount() == 4 ) );
	assert( pro );
	d_pro = pro;
	d_pro->addObserver( this );

	d_orig = spec;

    d_src2D = new SpinPointSpace( pro->getSpins(),
                                 pro->getRepository()->getTypes(), false,
                                 true, true, false );
    d_src4D = new SpinPointSpace( pro->getSpins(), pro->getRepository()->getTypes(), false,
                                 true, true, false );
	d_mdl2D = new LinkFilterRotSpace( d_src2D );
	d_mdl4D = new LinkFilterRotSpace( d_src4D );

	initParams();
	createPopup();
	initViews();
    d_cursor[ DimX ] = spec->getScale( DimX ).getRange().first;
    d_cursor[ DimY ] = spec->getScale( DimY ).getRange().first;
	if( spec->getDimCount() == 2 )
        setSpec2D( spec );
    else
	{
        d_cursor[ DimZ ] = spec->getScale( DimZ ).getRange().first;
        d_cursor[ DimW ] = spec->getScale( DimW ).getRange().first;
        d_show4DPlane = true;
        setSpec4D( spec );
	}
	updateSpecPop2D();
	updateSpecPop4D();
}

FourDScopeAgent::~FourDScopeAgent()
{
	d_pro->removeObserver( this );
	d_pro = 0;
	
	if( d_popSpec2D )
		delete d_popSpec2D;
	if( d_popSpec4D )
		delete d_popSpec4D;
	if( d_popPlane )
		delete d_popPlane;
	if( d_popOrtho )
		delete d_popOrtho;

	GlobalCursor::removeObserver( this );
}

void FourDScopeAgent::initViews()
{
	d_lock = true;
	d_cursor.assign( 4, 0 );
	d_slices.assign( 4, SliceSocket() );

	createPlane();
	createSlice( DimX, DimX );
	createSlice( DimY, DimY );
    createSlice( DimY, DimZ );
    createSlice( DimX, DimW );
    createOrtho();
    d_lock = false;

	// d_plane.d_ol[0].d_buf->fitToArea(); wird verzgert gemacht
	updatePlaneLabel();
}

void FourDScopeAgent::createPopup()
{
	d_popSpec2D = new Gui::Menu();
	d_popSpec4D = new Gui::Menu();

    d_popPlane = new Gui::Menu();

	Gui::Menu* menuPeaks = new Gui::Menu( d_popPlane );
	Gui::Menu::item( menuPeaks, this, "&Pick Peak", PickPlPeak, false );
	Gui::Menu::item( menuPeaks, this, "&Move Peak", MovePlPeak, false );
	Gui::Menu::item( menuPeaks, this, "&Move Peak Alias", MovePlAlias, false );
	Gui::Menu::item( menuPeaks, this, "&Label Peak...", LabelPlPeak, false );
	Gui::Menu::item( menuPeaks, this, "&Delete Peaks", DeletePlPeaks, false );
	Gui::Menu::item( menuPeaks, this, "&Edit Attributes...", EditPlPeakAtts, false );
	menuPeaks->insertSeparator();
	Gui::Menu::item( menuPeaks, this, "&Open Peaklist...", OpenPeakList, false );

	Gui::Menu::item( d_popPlane, this, "Hold Reference", HoldReference, true );
	Gui::Menu::item( d_popPlane, this, "Add Vertical Ruler", AddRulerHori, false );
	Gui::Menu::item( d_popPlane, this, "Add Horizontal Ruler", AddRulerVerti, false );
	Gui::Menu::item( d_popPlane, this, "Remove All Rulers", RemoveAllRulers, false );
	d_popPlane->insertItem( "Select Spectrum", d_popSpec2D );
	d_popPlane->insertItem( "Peaks", menuPeaks );
	d_popPlane->insertSeparator();
    Gui::Menu::item( d_popPlane, this, "&Pick New System", PickSystem, false );
    Gui::Menu::item( d_popPlane, this, "Propose System...", ProposePeak, false );
    Gui::Menu::item( d_popPlane, this, "Extend Horizontally...", PickHori, false );
    Gui::Menu::item( d_popPlane, this, "Propose Horizontally...", ProposeHori, false );
    Gui::Menu::item( d_popPlane, this, "Extend Vertically...", PickVerti, false );
    Gui::Menu::item( d_popPlane, this, "Propose Vertically...", ProposeVerti, false );
    Gui::Menu::item( d_popPlane, this, "&Move Spins", MovePeak, false );
    Gui::Menu::item( d_popPlane, this, "Move Spin &Aliases", MovePeakAlias, false );
    Gui::Menu::item( d_popPlane, this, "&Label Spins...", LabelPeak, false );
    Gui::Menu::item( d_popPlane, this, "Hide/Show Link", HidePeak, false );
    Gui::Menu::item( d_popPlane, this, "Set System &Type...", SetSystemType, false );
    Gui::Menu::item( d_popPlane, this, "Set Link Params...", SetLinkParams, false );

	Gui::Menu* menuAtts = new Gui::Menu( d_popPlane );
	Gui::Menu::item( menuAtts, this, "Horizontal Spin...", EditAttsSpinH, false );
	Gui::Menu::item( menuAtts, this, "Vertical Spin...", EditAttsSpinV, false );
	Gui::Menu::item( menuAtts, this, "Horizontal System...", EditAttsSysH, false );
	Gui::Menu::item( menuAtts, this, "Vertical System...", EditAttsSysV, false );
	Gui::Menu::item( menuAtts, this, "Spin Link...", EditAttsLink, false );
	d_popPlane->insertItem( "&Edit Attributes", menuAtts );

	d_popPlane->insertSeparator();
    Gui::Menu::item( d_popPlane, this, "Un-Alias Peaks", DeleteAliasPeak, false );
    Gui::Menu::item( d_popPlane, this, "Delete Peaks", DeletePeak, false );
    Gui::Menu::item( d_popPlane, this, "Delete Spin Links", DeleteLinks, false );
    Gui::Menu::item( d_popPlane, this, "Delete Horizontal Spin", DeleteSpinX, false );
	Gui::Menu::item( d_popPlane, this, "Delete Vertical Spin", DeleteSpinY, false );
	d_popPlane->insertSeparator();

    // Gui::Menu::item( d_popPlane, this, "Forward", Forward, false );
	Gui::Menu::item( d_popPlane, this, "Set Peak Width...", SetWidth, false );
	Gui::Menu::item( d_popPlane, this, "Show Alignment...", ShowAlignment, false );
	Gui::Menu::item( d_popPlane, this, "Backward", Backward, false );
	Gui::Menu::item( d_popPlane, this, "Show 4D Plane", Show4dPlane, true );
	Gui::Menu::item( d_popPlane, this, "Fit Window", FitWindow, false );

	d_popOrtho = new Gui::Menu();

	d_popOrtho->insertItem( "Select Spectrum", d_popSpec4D );
	d_popOrtho->insertItem( "Peaks", menuPeaks );
	d_popOrtho->insertSeparator();
    Gui::Menu::item( d_popOrtho, this, "&Pick Spins", PickSpin4D, false );
    Gui::Menu::item( d_popOrtho, this, "Pick Labels...", PickLabel4D, false );
    Gui::Menu::item( d_popOrtho, this, "Extend Horizontally...", PickHori4D, false );
    Gui::Menu::item( d_popOrtho, this, "Extend Vertically...", PickVerti4D, false );
    Gui::Menu::item( d_popOrtho, this, "&Propose Spins...", ProposeSpin, false );
    Gui::Menu::item( d_popOrtho, this, "&Move Spins", MoveSpin4D, false );
    Gui::Menu::item( d_popOrtho, this, "&Move Spin Aliasses", MoveSpinAlias4D, false );
    Gui::Menu::item( d_popOrtho, this, "Label Spins...", LabelSpin4D, false );
    Gui::Menu::item( d_popOrtho, this, "Set Link Params...", SetLinkParams4D, false );
    d_popOrtho->insertSeparator();
    Gui::Menu::item( d_popOrtho, this, "&Delete Spins", DeleteSpins4D, false ); // TODO: Delete X und Y
    Gui::Menu::item( d_popOrtho, this, "Delete Horizontal Spin", DeleteSpinX4D, false );
	Gui::Menu::item( d_popOrtho, this, "Delete Vertical Spin", DeleteSpinY4D, false );
    Gui::Menu::item( d_popOrtho, this, "Delete Links", DeleteLinks4D, false );

    menuAtts = new Gui::Menu( d_popOrtho );
    Gui::Menu::item( menuAtts, this, "Horizontal Spin...", EditAttsSpinX4D, false );
	Gui::Menu::item( menuAtts, this, "Vertical Spin...", EditAttsSpinY4D, false );
	Gui::Menu::item( menuAtts, this, "System...", EditAttsSys4D, false );
	Gui::Menu::item( menuAtts, this, "Spin Link...", EditAttsLink4D, false );
	d_popOrtho->insertItem( "&Edit Attributes", menuAtts );
	d_popOrtho->insertSeparator();
	Gui::Menu::item( d_popOrtho, this, "Set Peak Width...", SetWidth, false );
	Gui::Menu::item( d_popOrtho, this, "Show 4D Plane", Show4dPlane, true );
    Gui::Menu::item( d_popOrtho, this, "Show Path Simulation...", ShowPathSim, false );
	Gui::Menu::item( d_popOrtho, this, "Fit Window", FitWindow4D, false );
}

void FourDScopeAgent::updateSpecPop2D()
{
	if( d_popSpec2D == 0 )
		return;

	d_popSpec2D->purge();
	ColorMap a, b;
    if( !d_spec2D.isNull() )
        d_spec2D->getColors( a );
    else if( !d_spec4D.isNull() )
        d_spec4D->getColors( a );
	Project::SpecSet l;
	Spectrum* spec = 0;
	const Project::SpectrumMap& sm = d_pro->getSpectra();
	Project::SpectrumMap::const_iterator p;
	if( d_orig->getDimCount() == 2 )
		l.insert( d_orig );
	for( p = sm.begin(); p != sm.end(); ++p )
	{
		spec = (*p).second;
		spec->getColors( b );
		if( spec->getDimCount() == 2 && 
			a[ DimX ] == b[ DimX ] && a[ DimY ] == b[ DimY ] &&
			spec->getId() != d_orig->getId() )
			l.insert( spec );
	}
	if( d_spec2D == d_spec4D )
	{
		Gui::Menu::item( d_popSpec2D, this, d_spec4D->getName(),
			FourDScopeAgent::SelectSpec2D, true )->addParam( Root::Any( d_spec4D ) );
	}
	Sort::const_iterator pp1;
	Project::SpecSet::const_iterator p1;
	for( p1 = l.begin(); p1 != l.end(); ++p1 )
		d_sort2D[ (*p1)->getName() ] = (*p1);
	for( pp1 = d_sort2D.begin(); pp1 != d_sort2D.end(); ++pp1 )
	{
		Gui::Menu::item( d_popSpec2D, this, (*pp1).first.data(), 
			FourDScopeAgent::SelectSpec2D, 
			true )->addParam( Root::Any( (*pp1).second ) );
	}
}

void FourDScopeAgent::updateSpecPop4D()
{
	d_popSpec4D->purge();
	ColorMap a, b;
    if( !d_spec2D.isNull() )
        d_spec2D->getColors( a );
    else if( !d_spec4D.isNull() )
        d_spec4D->getColors( a );
	Project::SpecSet l;
	Spectrum* spec = 0;
	if( a.size() == 4 )
    {
		a[ DimZ ] = AtomType(); // Joker
        a[ DimW ] = AtomType(); // Joker
	}else
    {
		a.push_back( AtomType() );
        a.push_back( AtomType() );
    }

    const Project::SpectrumMap& sm = d_pro->getSpectra();
    Project::SpectrumMap::const_iterator p;
    Rotation rot;
    if( d_orig->getDimCount() == 4 )
        l.insert( d_orig );
    for( p = sm.begin(); p != sm.end(); ++p )
    {
        spec = (*p).second;
        if( spec->getDimCount() == 4 && spec->getId() != d_orig->getId() )
        {
            spec->getColors( b );
            if( a[ DimX ] == b[ DimX ] && a[ DimY ] == b[ DimY ] )
                l.insert( spec );
            else
            {
                if( SpectrumType::autoRotate( a, spec, rot, false ) ) // Keine Auflsungsoptimierung
                {
                    l.insert( new SpecRotator( spec, rot ) );
                }
            }
        }
    }
	Project::SpecSet::const_iterator p1;
	Sort::const_iterator pp1;
	for( p1 = l.begin(); p1 != l.end(); ++p1 )
		d_sort4D[ (*p1)->getName() ] = (*p1);
	for( pp1 = d_sort4D.begin(); pp1 != d_sort4D.end(); ++pp1 )
	{
		Gui::Menu::item( d_popSpec4D, this, (*pp1).first.data(),
			FourDScopeAgent::SelectSpec4D, true )->addParam( Root::Any( (*pp1).second ) );
	}
}

void FourDScopeAgent::createPlane()
{
	d_plane.d_ol.assign( 1, PlaneSocket::Overlay() );

	d_plane.d_viewer = new SpecViewer( new ViewAreaMdl( true, true, true, true ), VIEWCOUNT );
	d_plane.d_viewer->getViewArea()->addObserver( this );

    if( d_spec2D )
        d_plane.d_ol[0].d_spec = new SpecProjector( d_spec2D, DimX, DimY );
	d_plane.d_ol[0].d_buf = new SpecBufferMdl( 
		d_plane.d_viewer->getViewArea(), d_plane.d_ol[0].d_spec, false );
	d_plane.d_ol[0].d_buf->setFolding( d_folding, false );

	d_plane.d_intens = new IntensityView( d_plane.d_ol[0].d_buf, false );
	d_plane.d_viewer->getViews()->replace( INTENSITY, d_plane.d_intens );

	d_plane.d_ol[0].d_view = new ContourView( d_plane.d_ol[0].d_buf, d_autoContour );
	Lexi::Layer* l = new Lexi::Layer();
	l->append( d_plane.d_ol[0].d_view );
	d_plane.d_viewer->getViews()->replace( CONTOUR, l );
	d_plane.d_ol[0].d_view->setVisi( s_showContour );
	const Repository::SlotColors& sc = d_pro->getRepository()->getScreenClr();
	if( !sc.empty() )
	{
		d_plane.d_ol[0].d_view->setPosColor( sc[ 0 ].d_pos );
		d_plane.d_ol[0].d_view->setNegColor( sc[ 0 ].d_neg );
	}
	d_plane.d_ol[0].d_view->createLevelsAuto( s_contourFactor, s_contourOption, s_gain );
	
	d_plane.d_cur = new CursorMdl();
	d_plane.d_cur->addObserver( this );
	d_plane.d_mdl4D = new RangeFilterSpaceND( d_mdl4D, DimZ, DimW );
	d_plane.d_tuples = new SpinPointView( d_plane.d_viewer, d_mdl2D );
	d_plane.d_tuples->setLabel( SpinPointView::PairLabelSysOrResi );
	d_plane.d_viewer->getViews()->replace( TUPLES, d_plane.d_tuples );
	CursorView* cv = new CursorView( d_plane.d_viewer, d_plane.d_cur );
	d_plane.d_viewer->getViews()->replace( CURSORS, cv );
	if( d_folding )
		d_plane.d_viewer->getViews()->replace( FOLDING, new FoldingView( d_plane.d_ol[0].d_buf ) );
	d_plane.d_hRuler = new SpinPoint1DView( d_plane.d_viewer, DimY, 0, ( Qt::darkYellow) );
    d_plane.d_hRulerMdl = new ManualSpinSpace( 1 );
	d_plane.d_hRuler->setModel( d_plane.d_hRulerMdl );
	d_plane.d_hRuler->setLabel( SpinPoint1DView::SysTagAll );
	d_plane.d_viewer->getViews()->replace( RULER1, d_plane.d_hRuler );
	d_plane.d_vRuler = new SpinPoint1DView( d_plane.d_viewer, DimX, 0, ( Qt::darkYellow) );
    d_plane.d_vRulerMdl = new ManualSpinSpace( 1 );
	d_plane.d_vRuler->setModel( d_plane.d_vRulerMdl );
	d_plane.d_vRuler->setLabel( SpinPoint1DView::SysTagAll );
	d_plane.d_viewer->getViews()->replace( RULER2, d_plane.d_vRuler );

    d_plane.d_pp = new PeakSubSpaceND( new PeakSpaceDummy( s_defDim ), DimX, DimY );
	d_plane.d_peaks = new PeakPlaneView( d_plane.d_viewer, d_plane.d_pp );
    d_plane.d_peaks->setColor( g_clrPeak );
	d_plane.d_viewer->getViews()->replace( PEAKS, d_plane.d_peaks );

	d_plane.d_viewer->getHandlers()->append( new ZoomCtrl( d_plane.d_viewer, true, true ) );
	d_plane.d_viewer->getHandlers()->append( new SelectZoomCtrl( d_plane.d_viewer, true, true ) );
	d_plane.d_viewer->getHandlers()->append( new ScrollCtrl( d_plane.d_viewer ) );
	d_plane.d_viewer->getHandlers()->append( new CursorCtrl( cv, false ) );
	d_plane.d_viewer->getHandlers()->append( new PointSelect1DCtrl( d_plane.d_hRuler, false ) );
	d_plane.d_viewer->getHandlers()->append( new PointSelect1DCtrl( d_plane.d_vRuler, false ) );
	// NOTE: Select muss vor Cursor kommen, da sonst Selection zu spt passiert.
	d_plane.d_viewer->getHandlers()->append( new PointSelectCtrl( d_plane.d_tuples, false ) );
	d_plane.d_viewer->getHandlers()->append( new PeakSelectCtrl( d_plane.d_peaks, false, false ) ); // Kein Drag-Select
	d_plane.d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_popPlane, false ) );
	d_plane.d_viewer->getHandlers()->append( new FocusCtrl( d_plane.d_viewer ) );
}

void FourDScopeAgent::initOverlay(int n)
{
	assert( n > 0 );
	if( d_plane.d_ol.size() == n )
		return;
	Lexi::Glyph* g = d_plane.d_viewer->getViews()->getComponent( CONTOUR );
	while( g->getCount() )
		g->remove( 0 );

	int i;
	if( n > d_plane.d_ol.size() )
	{
		int old = d_plane.d_ol.size();
		d_plane.d_ol.resize( n );
		const Repository::SlotColors& sc = d_pro->getRepository()->getScreenClr();
		for( i = old; i < n; i++ )
		{
			d_plane.d_ol[i].d_buf = new SpecBufferMdl( 
				d_plane.d_viewer->getViewArea() );
			d_plane.d_ol[i].d_buf->setFolding( d_folding );
			d_plane.d_ol[i].d_view = new ContourView( d_plane.d_ol[i].d_buf, d_autoContour );
			if( i < sc.size() )
			{
				d_plane.d_ol[i].d_view->setPosColor( sc[ i ].d_pos );
				d_plane.d_ol[i].d_view->setNegColor( sc[ i ].d_neg );
			}
			d_plane.d_ol[i].d_view->setVisi( d_plane.d_ol[0].d_view->isVisi() );
			d_plane.d_ol[i].d_view->createLevelsAuto( s_contourFactor, s_contourOption, s_gain );
		}
	}else
	{
		d_plane.d_ol.resize( n );
	}
	for( i = 0; i < d_plane.d_ol.size(); i++ )
		g->append( d_plane.d_ol[i].d_view );
	setActiveOverlay( 0 );
	updatePlaneLabel();
}

void FourDScopeAgent::setActiveOverlay(int n)
{
	if( n == d_aol )
		return;
	d_plane.d_viewer->redraw();

	d_aol = n;
	updatePlaneLabel();
}

int FourDScopeAgent::selectLayer()
{
	if( d_plane.d_ol.size() == 1 )
		return 0;

	Dlg::StringList l( d_plane.d_ol.size() + 1);
	l[ 0 ] = "&All";
	QString str;
	for( int i = 0; i < d_plane.d_ol.size(); i++ )
	{
		if( d_plane.d_ol[ i ].d_spec )
			str.sprintf( "&%d %s", i, d_plane.d_ol[ i ].d_spec->getName() );
		else
			str.sprintf( "&%d <empty>", i );
		l[ i + 1 ] = str.toLatin1();
	}
	int c = Dlg::getOption( MW, l, 
		"Select Layer", d_aol + 1 );
	if( c == -1 )
		return -2;
	else
		return c - 1;
}

void FourDScopeAgent::createOrtho()
{
	d_ortho.d_viewer = new SpecViewer(
		new ViewAreaMdl( true, true, true, true ), VIEWCOUNT );
	d_ortho.d_viewer->getViewArea()->addObserver( this );

	d_ortho.d_buf = new SpecBufferMdl( d_ortho.d_viewer->getViewArea(), 0, false );
	d_ortho.d_buf->setFolding( d_folding, false );

	d_ortho.d_view = new ContourView( d_ortho.d_buf, true );	// Immer auto
	d_ortho.d_viewer->getViews()->replace( BACKGROUND, new Lexi::Background() );
	d_ortho.d_viewer->getViews()->replace( CONTOUR, d_ortho.d_view );
	d_ortho.d_view->createLevelsAuto( s_contourFactor, s_contourOption, s_gain );

	d_ortho.d_cur = new CursorMdl();
	d_ortho.d_cur->addObserver( this );
	Rotation rot( 4 );
	rot[ DimX ] = DimW;
	rot[ DimY ] = DimZ;
	rot[ DimZ ] = DimX;
    rot[ DimW ] = DimY;
	d_ortho.d_mdl = new RangeFilterSpaceND( d_mdl4D, DimX, DimY );
	d_ortho.d_tuples = new SpinPointView( d_ortho.d_viewer, new RotatedSpace( d_ortho.d_mdl, rot ) );
	d_ortho.d_tuples->setLabel( SpinPointView::PairLabelSysOrResi );
	d_ortho.d_viewer->getViews()->replace( TUPLES, d_ortho.d_tuples );
	CursorView* cv = new CursorView( d_ortho.d_viewer, d_ortho.d_cur, true, true );
	d_ortho.d_viewer->getViews()->replace( CURSORS, cv );
	if( d_folding )
		d_ortho.d_viewer->getViews()->replace( FOLDING, new FoldingView( d_ortho.d_buf ) );

	d_ortho.d_pp = new PeakSubSpaceND( new PeakSpaceDummy( s_defDim ), DimW, DimZ );
	d_ortho.d_peaks = new PeakPlaneView( d_ortho.d_viewer, d_ortho.d_pp );
	d_ortho.d_peaks->setColor( g_clrPeak );
	d_ortho.d_viewer->getViews()->replace( PEAKS, d_ortho.d_peaks );

	d_ortho.d_viewer->getHandlers()->append( new ZoomCtrl( d_ortho.d_viewer, true, true ) );
	d_ortho.d_viewer->getHandlers()->append( new SelectZoomCtrl( d_ortho.d_viewer, true, true ) );
	d_ortho.d_viewer->getHandlers()->append( new ScrollCtrl( d_ortho.d_viewer, true, true ) );
	d_ortho.d_viewer->getHandlers()->append( new CursorCtrl( cv, false, true, true ) );
	d_ortho.d_viewer->getHandlers()->append( new PointSelectCtrl( d_ortho.d_tuples, false ) );
	d_ortho.d_viewer->getHandlers()->append( new PeakSelectCtrl( d_ortho.d_peaks, false, false ) ); // Kein Drag-Select
	d_ortho.d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_popOrtho, false ) );
	d_ortho.d_viewer->getHandlers()->append( new FocusCtrl( d_ortho.d_viewer ) );
}

void FourDScopeAgent::createSlice(Dimension view, Dimension spec)
{
	SpecViewer* slice = new SpecViewer( 
		new ViewAreaMdl( view == DimX, view == DimY, view == DimX, view == DimY ) );
	d_slices[ spec ].d_viewer = slice;
	slice->getViewArea()->addObserver( this );

	d_slices[ spec ].d_buf2D = new SpecBufferMdl( slice->getViewArea(), 0, false );
	d_slices[ spec ].d_buf2D->setFolding( d_folding, false );
	slice->getViews()->append( new SliceView( d_slices[ spec ].d_buf2D ) );

	d_slices[ spec ].d_buf4D = new SpecBufferMdl( slice->getViewArea(), 0, false );
	d_slices[ spec ].d_buf4D->setFolding( d_folding, false );
	slice->getViews()->append( new SliceView( d_slices[ spec ].d_buf4D, g_clrSlice4D ) );

	d_slices[ spec ].d_cur = new CursorMdl();
	d_slices[ spec ].d_cur->addObserver( this );
	CursorView* cv = new CursorView( slice, d_slices[ spec ].d_cur );
	slice->getViews()->append( cv );

	slice->getHandlers()->append( new ZoomCtrl( slice, view == DimX, view == DimY ) );
	slice->getHandlers()->append( new SelectZoomCtrl( slice, view == DimX, view == DimY ) );
	slice->getHandlers()->append( new ScrollCtrl( slice ) );
	slice->getHandlers()->append( new SelectRulerCtr( slice, true ) );
	slice->getHandlers()->append( new CursorCtrl( cv, false ) );
	slice->getHandlers()->append( new FocusCtrl( slice ) );
}

void FourDScopeAgent::updateContour( int i, bool redraw )
{
	if( !d_plane.d_ol[0].d_view->isVisi() )
		return;

	if( d_show4DPlane && i == 0 )
	{
		if( d_ortho.d_view->isAuto() )
		{
			d_plane.d_ol[0].d_view->createLevelsAuto( d_ortho.d_view->getFactor(),
				d_ortho.d_view->getOption(), d_ortho.d_view->getGain() );
		}else
			d_plane.d_ol[0].d_view->createLevelsMin( d_ortho.d_view->getFactor(),
				(d_spec4D)?d_spec4D->getThreshold():0.0, d_ortho.d_view->getOption() );
	}else
	{
		if( d_plane.d_ol[i].d_view->isAuto() )
		{
			d_plane.d_ol[i].d_view->createLevelsAuto();
		}else if( d_plane.d_ol[i].d_spec )
			d_plane.d_ol[i].d_view->createLevelsMin( d_plane.d_ol[i].d_spec->getThreshold() );
	}
	if( redraw )
		d_plane.d_viewer->damageMe();
}

void FourDScopeAgent::showIntens(bool on )
{
	if( d_plane.d_intens->isVisi() == on )
		return;
	Lexi::Viewport::pushHourglass();
	d_plane.d_intens->setVisi( on );
	Lexi::Viewport::popCursor();
}

void FourDScopeAgent::initParams()
{
	d_resol = 1;
	d_lowResol = false;
	d_autoContour = true;
	d_autoCenter = true;
	d_contourFactor = 1.4f;
	d_contourOption = ContourView::Both;
	d_folding = false;
	d_show4DPlane = false;
	d_autoHide = true;
	d_cursorSync = false;
	d_gain = 2.0;
	d_rangeSync = false;
	d_autoHold = false;
	d_syncDepth = true;
	// TODO: diese Werte sollen ab Konfigurations-Record gelesen werden
}

void FourDScopeAgent::centerToCursor(bool threeD)
{
	if( !threeD )
	{
		ViewAreaMdl* area = d_plane.d_viewer->getViewArea();
		if( !area->getRange( DimX ).contains( d_cursor[ DimX ] ) ||
			!area->getRange( DimY ).contains( d_cursor[ DimY ] ) )
		{
			area->centerPoint( d_cursor[ DimX ], d_cursor[ DimY ] );
			d_plane.d_viewer->damageMe();
		}
	}else
	{
		ViewAreaMdl* area = d_ortho.d_viewer->getViewArea();
		if( !area->getRange( DimY ).contains( d_cursor[ DimZ ] ) ||
            !area->getRange( DimX ).contains( d_cursor[ DimW ] ) )
		{
			area->centerPoint( d_cursor[ DimW ], d_cursor[ DimZ ] );
			d_ortho.d_viewer->damageMe();
		}
	}
}

void FourDScopeAgent::handle(Root::Message& msg)
{
	if( d_lock )
		return;
	d_lock = true;
	BEGIN_HANDLER();
	MESSAGE( ViewAreaMdl::Update, a, msg )
	{
		Lexi::Viewport::pushHourglass();
		if( a->getOrigin() == d_plane.d_viewer->getViewArea() )
			planeAreaUpdated( a );
		else
		{
			Dimension d;
			for( d = 0; d < d_slices.size(); d++ )
				if( d_slices[ d ].d_viewer->getViewArea() == a->getOrigin() )
				{
					sliceAreaUpdated( d, a );
					break;
				}
            if( d_ortho.d_viewer->getViewArea() == a->getOrigin() )
            {
                orthoAreaUpdated( a );
            }
        }

		Lexi::Viewport::popCursor();
		msg.consume();
	}
	MESSAGE( CursorMdl::Update, a, msg )
	{
		if( a->getOrigin() == d_plane.d_cur )
			planeCursUpdated( a );
		else
		{
			Dimension d;
			for( d = 0; d < d_slices.size(); d++ )
				if( d_slices[ d ].d_cur == a->getOrigin() )
				{
					sliceCursUpdated( d, a );
					break;
				}
            if( d_ortho.d_cur == a->getOrigin() )
            {
                orthoCursUpdated( a );
            }
        }
		msg.consume();
	}
	MESSAGE( GlobalCursor::UpdatePos, a, msg )
	{
		d_lock = false;
		d_cursorSync = false;
		bool fourD = false;
		if( ( a->getDim() == DimY || a->getDim() == DimUndefined ) &&
			d_plane.d_ol[0].d_spec->getColor( DimY ) == a->getTy() )
			d_plane.d_cur->setCursor( Dimension( DimY ), a->getY() );
		if( ( a->getDim() == DimX || a->getDim() == DimUndefined ) &&
			d_plane.d_ol[0].d_spec->getColor( DimX ) == a->getTx() )
			d_plane.d_cur->setCursor( Dimension( DimX ), a->getX() );
		if( d_syncDepth && ( a->getDim() == DimY || a->getDim() == DimUndefined ) && d_spec4D &&
			d_spec4D->getColor( DimZ ) == a->getTy() )
		{
			d_cursor[ DimZ ] = a->getY();
            d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, a->getY() );
            d_ortho.d_cur->setCursor( (Dimension)DimY, a->getY() );
            fourD = true;
		}
        if( d_syncDepth && ( a->getDim() == DimX || a->getDim() == DimUndefined ) && d_spec4D &&
			d_spec4D->getColor( DimW ) == a->getTx() )
		{
			d_cursor[ DimW ] = a->getX();
            d_slices[ DimW ].d_cur->setCursor( (Dimension)DimX, a->getX() );
            d_ortho.d_cur->setCursor( (Dimension)DimX, a->getX() );
            fourD = true;
		}
		d_cursorSync = true;
		if( !fourD )
		{
			selectCurSystem();
			if( !d_rangeSync )
				centerToCursor( fourD );
		}else
			centerToCursor( fourD );
		msg.consume();
	}
	MESSAGE( GlobalCursor::UpdateRange, a, msg )
	{
		d_lock = false;
		d_rangeSync = false;
		if( ( a->getDim() == DimY || a->getDim() == DimUndefined ) &&
			d_plane.d_ol[0].d_spec->getColor( DimY ) == a->getTy() )
			d_plane.d_viewer->getViewArea()->setRange( DimY, a->getY() );
		if( ( a->getDim() == DimX || a->getDim() == DimUndefined ) &&
			d_plane.d_ol[0].d_spec->getColor( DimX ) == a->getTx() )
			d_plane.d_viewer->getViewArea()->setRange( DimX, a->getX() );
		d_rangeSync = true;
		msg.consume();
	}
	MESSAGE( Root::Action, a, msg )
	{
		d_lock = false; // Kein Blocking fr Action-Ausfhrung
		EXECUTE_ACTION( FourDScopeAgent, *a );
	}
	MESSAGE( Project::Changed, a, msg )
	{
		msg.consume();
		if( a->d_hint == Project::Width )
		{
			if( d_spec4D )
				d_plane.d_mdl4D->setGhostWidth( d_pro->inferPeakWidth( DimZ, d_spec4D ),
                                            d_pro->inferPeakWidth( DimW, d_spec4D ) );
			if( d_show4DPlane )
				d_plane.d_pp->setDimGhostWidth( DimZ, DimW, d_pro->inferPeakWidth( DimZ, d_spec4D ),
                                        d_pro->inferPeakWidth( DimW, d_spec4D ) );
			syncOrthoToCur();
		}else if( a->d_hint == Project::WidthFactor )
		{
			syncOrthoToCur();
		}
	}
	MESSAGE( SpectrumPeer::Added, a, msg )
	{
        Q_UNUSED(a)
        updateSpecPop2D();
		updateSpecPop4D();
	}
	MESSAGE( SpectrumPeer::Removed, a, msg )
	{
        Q_UNUSED(a)
		updateSpecPop2D();
		updateSpecPop4D();
	}
	MESSAGE( Spin::Update, a, msg )
	{
		switch( a->getType() )
		{
		case Spin::Update::Delete:
			{
				for( int i = 0; i < d_ref.maxSize(); i++ )
				{
					if( d_ref[ i ] == a->getSpin() )
					{
						d_ref.zero();
						updateRef();
						break;
					}
				}
			}
			break;
		case Spin::Update::Label:
		case Spin::Update::System:
			{
				for( int i = 0; i < d_ref.maxSize(); i++ )
				{
					if( d_ref[ i ] == a->getSpin() )
					{
						updateRef();
						break;
					}
				}
			}
			break;
		case Spin::Update::All:
			d_ref.zero();
			updateRef();
			break;
        default:
            break;
		}
		msg.consume();
	}
	END_HANDLER();
	d_lock = false;
}

bool FourDScopeAgent::askToClosePeaklist()
{
    if( d_pl == 0 ||
        ( d_pl && ( d_pl->getId() != 0 || !d_pl->getPeakList()->isDirty() ) ) )
		return true;
	switch( QMessageBox::warning( MW, "About to close peaklist",
			"Do you want to save the peaklist in the repository before closing?",
										  "&Save", "&Don't Save", "&Cancel",
										  0,		// Enter == button 0
										  2 ) )		// Escape == button 2
	{
	case 0:
		return savePeakList(); // Do it with action.
	case 1:
		return true;	// Do it without action
	default:
		return false;	// Don't do it.
	}
}

bool FourDScopeAgent::savePeakList()
{
	bool ok;
	QString res = QInputDialog::getText( "Name Peaklist", 
		"Please enter a unique short name:", QLineEdit::Normal, 
		d_pl->getName().data(), &ok, MW );
	if( !ok )
		return false;
	if( res.isEmpty() || d_pro->findPeakList( res.toLatin1() ) != 0 )	
	{
		QMessageBox::critical( MW, "Save Peaklist",
				"This peaklist name is already used!", "&Cancel" );
		return false;
	}
	d_pl->getPeakList()->setName( res.toLatin1() );
	d_pro->addPeakList( d_pl->getPeakList() );
	d_pl->getPeakList()->clearDirty();
	return true;
}

void FourDScopeAgent::planeCursUpdated(CursorMdl::Update * msg)
{
	// Auf der Plane wurde der Cursor ge�dert
	assert( d_slices.size() >= 2 );
	PpmPoint pos( msg->getX(), msg->getY() );

	if( d_autoCenter && msg->getDim() == DimUndefined )
	{
		Spectrum* spec = d_spec2D;
		if( d_spec4D && d_show4DPlane )
			spec = d_spec4D;
		SpinPoint tuple = 
			d_plane.d_tuples->getHit( pos[ DimX ], pos[ DimY ] );
		if( !tuple.isZero() )
		{
			pos[ DimX ] = tuple[ DimX ]->getShift( spec ); 
			pos[ DimY ] = tuple[ DimY ]->getShift( spec ); 
			msg->override( pos[ DimX ], pos[ DimY ] ); 
		}else
		{
			Root::Index peak = d_plane.d_peaks->getHit( pos[ DimX ], pos[ DimY ] );
			if( peak )
			{
				PeakPos p;
				d_plane.d_peaks->getModel()->getPos( peak, p, spec );
				msg->override( p[ DimX ], p[ DimY ] ); 
				pos[ DimX ] = p[ DimX ];
				pos[ DimY ] = p[ DimY ];
				// das geht, da View die Message erst nach Agent erh�t.
			}
		}
	}

	// Der X-Slice zeigt den durch den Y-Cursor der Plane
	// reprsentierten Slice mit Origin Y (und umgekehrt)
	if( msg->getDim() == DimY || msg->getDim() == DimUndefined )
	{
		d_cursor[ DimY ] = pos[ DimY ];
		d_slices[ DimY ].d_cur->setCursor( (Dimension)DimY, pos[ DimY ] );
        if( d_slices[ DimX ].d_spec2D )
            d_slices[ DimX ].d_spec2D->setOrigin( DimY, pos[ DimY ] ); // der andere
		d_slices[ DimX ].d_viewer->redraw();
		sync4dXySliceToCur( DimX, false );
		if( d_cursorSync )
			GlobalCursor::setCursor( DimY, pos[ DimY ], d_plane.d_ol[0].d_spec->getColor( DimY ) );
	}
	if( msg->getDim() == DimX || msg->getDim() == DimUndefined )
	{
		d_cursor[ DimX ] = pos[ DimX ];
		d_slices[ DimX ].d_cur->setCursor( (Dimension)DimX, pos[ DimX ] );
        if( d_slices[ DimY ].d_spec2D )
            d_slices[ DimY ].d_spec2D->setOrigin( DimX, pos[ DimX ] ); // der andere
		d_slices[ DimY ].d_viewer->redraw();
		sync4dXySliceToCur( DimY, false );
		if( d_cursorSync )
			GlobalCursor::setCursor( DimX, pos[ DimX ], d_plane.d_ol[0].d_spec->getColor( DimX ) );
	}
	sync4dZwSliceToCur();
	syncOrthoToCur();
	notifyCursor();
}

void FourDScopeAgent::sliceCursUpdated(Dimension dim, CursorMdl::Update *msg)
{
	// Auf einem Slice wurde der Cursor gendert
	d_cursor[ dim ] = msg->getX();
	if( dim == DimX || dim == DimY )
	{
		// X/Y-Slice gecklickt
		d_plane.d_cur->setCursor( dim, msg->getX() ); // Beide Dims gleich
		syncOrthoToCur();
		sync4dZwSliceToCur();
        if( dim == DimX )
        {
            sync4dXySliceToCur( DimY ); // der andere
        }else if( dim == DimY )
        {
            sync4dXySliceToCur( DimX ); // der andere
        }
		notifyCursor();
		if( d_cursorSync )
			GlobalCursor::setCursor( dim, msg->getX(), d_slices[ dim ].d_spec2D->getColor( DimX ) );
	}else if( dim == DimZ || dim == DimW )
	{
		// Z/W-Slice geklickt
		// registerPlane();
        if( dim == DimZ )
        {
            d_ortho.d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
            sync4dZwSliceToCur( DimW ); // der andere
        }else if( dim == DimW )
        {
            d_ortho.d_cur->setCursor( (Dimension)DimX, d_cursor[ DimW ] );
            sync4dZwSliceToCur( DimZ ); // der andere
        }
		sync4dXySliceToCur( true );
		notifyCursor( false );
		if( d_cursorSync && d_syncDepth )
        {
            if( dim == DimZ )
                GlobalCursor::setCursor( DimY, msg->getX(), d_spec4D->getColor( DimZ ) );
            else if( dim == DimW )
                GlobalCursor::setCursor( DimX, msg->getX(), d_spec4D->getColor( DimW ) );
        }
	}
}

void FourDScopeAgent::setCursor( PpmPoint p)
{
	if( p.size() == 0 )
	{
		p.assign( getSpec()->getDimCount(), 0 );
		for( int i = 0; i < p.size(); i++ )
			p[ i ] = getSpec()->getScale( i ).getIdxN();
	}
	for( Dimension d = 0; d < p.size(); d++ )
		d_cursor[ d ] = p[ d ];
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
    centerToCursor( false ); // RISK
	sync4dXySliceToCur(true);
    sync4dZwSliceToCur();
    syncOrthoToCur();
	// TODO: Selection
	notifyCursor( true );
}

void FourDScopeAgent::sync4dXySliceToCur(bool show)
{
	if( d_spec4D.isNull() )
		return;
    sync4dXySliceToCur( DimX, show );
	sync4dXySliceToCur( DimY, show );

	if( d_show4DPlane )
	{
		Lexi::Viewport::pushHourglass();
		d_plane.d_ol[0].d_spec->setOrigin( d_cursor );
		d_plane.d_viewer->redraw();
		Lexi::Viewport::popCursor();
	}
	assert( d_spec4D );
	Lexi::Viewport::pushHourglass();
    d_plane.d_mdl4D->setOrigThick( d_cursor[ DimZ ], d_cursor[ DimW ],
                               d_spec4D->getScale( DimZ ).getDelta(),
                               d_spec4D->getScale( DimW ).getDelta() );
	if( d_show4DPlane )
    {
		d_plane.d_pp->setOrigThick( DimZ, d_cursor[ DimZ ], d_spec4D->getScale( DimZ ).getDelta() );
        d_plane.d_pp->setOrigThick( DimW, d_cursor[ DimW ], d_spec4D->getScale( DimW ).getDelta() );
    }
	Lexi::Viewport::popCursor();
}

void FourDScopeAgent::sliceAreaUpdated(Dimension dim, ViewAreaMdl::Update *msg)
{
	// In Slice wurde Ausschnitt gendert
	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;
	if( dim < 2 )
	{
		// X/Y-Slice verndert
		d_plane.d_viewer->getViewArea()->setRange( dim, 
			d_slices[ dim ].d_viewer->getViewArea()->getRange( dim ) );
		d_plane.d_viewer->redraw();
		if( d_rangeSync )
			GlobalCursor::setRange( dim, d_slices[ dim ].d_viewer->getViewArea()->getRange( dim ), 
				d_slices[ dim ].d_spec2D->getColor( DimX ) );
	}else if( d_spec4D && ( dim == DimZ || dim == DimW ) )
	{
        if( dim == DimZ )
            d_ortho.d_viewer->getViewArea()->setRange( DimY,
               d_slices[ DimZ ].d_viewer->getViewArea()->getRange( DimY ) );
        else
            d_ortho.d_viewer->getViewArea()->setRange( DimX,
               d_slices[ DimW ].d_viewer->getViewArea()->getRange( DimX ) );
        d_ortho.d_viewer->redraw();
	}
}

void FourDScopeAgent::planeAreaUpdated(ViewAreaMdl::Update * msg)
{
	// In Plane wurde Ausschnitt ge�dert
	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;
	PpmCube cube;
	cube.assign( 2, PpmRange() );
	cube[ DimX ] = d_plane.d_viewer->getViewArea()->getRange( DimX );
	cube[ DimY ] = d_plane.d_viewer->getViewArea()->getRange( DimY );
	d_backward.push_back( std::make_pair( cube, d_cursor ) );

	if( msg->getX() )
	{
		d_slices[ DimX ].d_viewer->getViewArea()->setRange( DimX, 
			d_plane.d_viewer->getViewArea()->getRange( DimX ) );
		d_slices[ DimX ].d_viewer->redraw();
		if( d_rangeSync )
			GlobalCursor::setRange( DimX, 
			d_plane.d_viewer->getViewArea()->getRange( DimX ), 
			d_plane.d_ol[0].d_spec->getColor( DimX ) );
	}
	if( msg->getY() )
	{
		d_slices[ DimY ].d_viewer->getViewArea()->setRange( DimY, 
			d_plane.d_viewer->getViewArea()->getRange( DimY ) );
		d_slices[ DimY ].d_viewer->redraw();
		if( d_rangeSync )
			GlobalCursor::setRange( DimY, 
			d_plane.d_viewer->getViewArea()->getRange( DimY ), 
			d_plane.d_ol[0].d_spec->getColor( DimY ) );
	}
}

void FourDScopeAgent::orthoAreaUpdated(ViewAreaMdl::Update *msg)
{
	// In einem Strip wurde Ausschnitt gendert

	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;
	//registerPlane();

    if( msg->getY() )
	{
        d_slices[ DimZ ].d_viewer->getViewArea()->setRange( DimY,
           d_ortho.d_viewer->getViewArea()->getRange( DimY ) );
        d_slices[ DimZ ].d_viewer->redraw();
    }
    if( msg->getX() )
	{
        d_slices[ DimW ].d_viewer->getViewArea()->setRange( DimX,
           d_ortho.d_viewer->getViewArea()->getRange( DimX ) );
        d_slices[ DimW ].d_viewer->redraw();
    }
}

void FourDScopeAgent::orthoCursUpdated(CursorMdl::Update *msg)
{
	// In einem Strip wurde Cursor gendert

	if( d_spec4D.isNull() )
		return;

    PpmPoint pos( msg->getX(), msg->getY() );

	if( d_autoCenter && msg->getDim() == DimUndefined )
	{
        SpinPoint tuple = d_ortho.d_tuples->getHit( pos[ DimX ], pos[ DimY ] );
		if( !tuple.isZero() )
		{
            pos[ DimX ] = tuple[ DimX ]->getShift( d_spec4D );
			pos[ DimY ] = tuple[ DimY ]->getShift( d_spec4D );
			msg->override( pos[ DimX ], pos[ DimY ] );
		}else if( d_pl && d_pl->getDimCount() > 2 )
		{
			Root::Index peak = d_ortho.d_peaks->getHit( pos[ DimX ], pos[ DimY ] );
			if( peak )
			{
				PeakPos p;
				d_ortho.d_peaks->getModel()->getPos( peak, p, d_spec4D );
				msg->override( p[ DimX ], p[ DimY ] );
                pos[ DimX ] = p[ DimX ];
				pos[ DimY ] = p[ DimY ];
				// das geht, da View die Message erst nach Agent erh�t.
			}
		}
	}

	// registerPlane();
    if( msg->getDim() == DimX || msg->getDim() == DimUndefined )
	{
        d_cursor[ DimW ] = pos[ DimX ];
        d_slices[ DimW ].d_cur->setCursor( (Dimension)DimX, d_cursor[ DimW ] );
        sync4dZwSliceToCur(DimZ); // der andere
        if( d_cursorSync && d_syncDepth )
            GlobalCursor::setCursor( DimX, d_cursor[ DimW ], d_spec4D->getColor( DimW ) );
    }
    if( msg->getDim() == DimY || msg->getDim() == DimUndefined )
	{
        d_cursor[ DimZ ] = pos[ DimY ];
        d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
        sync4dZwSliceToCur(DimW); // der andere
        if( d_cursorSync && d_syncDepth )
            GlobalCursor::setCursor( DimY, d_cursor[ DimZ ], d_spec4D->getColor( DimZ ) );
    }
	sync4dXySliceToCur(true);
	if( d_show4DPlane )
		updateContour( 0, true );
	notifyCursor( false );
}

void FourDScopeAgent::updatePlaneLabel()
{
	Spectrum* spec = d_plane.d_ol[d_aol].d_spec;
	if( !d_spec4D.isNull() && d_show4DPlane )
		spec = d_spec4D;
	QString str;
    if( spec == 0 )
        str.sprintf( " %d <empty>", d_aol );
	else if( d_plane.d_ol.size() > 1 )
        str.sprintf( " %d %s  %s / %s", d_aol, spec->getName(), spec->getDimName( DimX ),
                     spec->getDimName( DimY ) );
    else
		str.sprintf( " %s  %s / %s", spec->getName(), spec->getDimName( DimX ),
			spec->getDimName( DimY ) );
	d_plane.d_viewer->getViews()->replace( LABEL1,
		new Lexi::Label( str, nil, 
		(d_show4DPlane)?g_clrSlice4D:d_plane.d_tuples->getColor(),
		Lexi::AlignLeft, Lexi::AlignTop ) );
}

void FourDScopeAgent::setSpec4D(Spectrum * spec)
{
    assert( spec == 0 || spec->getDimCount() == 4 );

	if( d_spec4D == spec )
		return;

	Lexi::Viewport::pushHourglass();
	Spectrum* old = d_spec4D;
	d_spec4D = spec;
	if( !d_spec4D.isNull() )
	{
		d_slices[ DimZ ].d_spec4D = new SpecProjector( d_spec4D, DimZ );
        d_slices[ DimW ].d_spec4D = new SpecProjector( d_spec4D, DimW );
		d_ortho.d_spec = new SpecProjector( d_spec4D, DimW, DimZ );

		d_slices[ DimZ ].d_buf4D->setSpectrum( d_slices[ DimZ ].d_spec4D, true );
        assert( d_slices[ DimZ ].d_spec2D.isNull() );

        d_slices[ DimW ].d_buf4D->setSpectrum( d_slices[ DimW ].d_spec4D, true );
        assert( d_slices[ DimW ].d_spec2D.isNull() );

		if( old == 0 || old->getColor( DimZ ) != spec->getColor( DimZ ) ||
			!old->getScale( DimZ ).getRange().intersects( spec->getScale( DimZ ).getRange() ) ||
			old->getColor( DimW ) != spec->getColor( DimW ) ||
			!old->getScale( DimW ).getRange().intersects( spec->getScale( DimW ).getRange()) )
		{
			// Nur FitWindow, wenn andere Farbe oder Bereich nicht berlappend
			PpmRange r = d_spec4D->getScale( DimZ ).getRange();
			r.invert();
            d_slices[ DimZ ].d_viewer->getViewArea()->setRange( DimY, r );
            d_slices[ DimW ].d_viewer->getViewArea()->setRange( DimX, d_spec4D->getScale( DimW ).getRange() );
			d_ortho.d_viewer->getViewArea()->setRange( d_spec4D->getScale( DimW ).getRange(), r );
		}

		d_mdl4D->setSpec( 0 );
		d_src4D->setSpec( d_spec4D );
		d_mdl4D->setSpec( d_spec4D );
		d_plane.d_mdl4D->setGhostWidth( d_pro->inferPeakWidth( DimZ, d_spec4D ),
                                    d_pro->inferPeakWidth( DimW, d_spec4D ) );
		syncOrthoToCur();

		d_ortho.d_viewer->getViews()->replace( LABEL1,
			new Lexi::Label( d_spec4D->getName(), nil, g_clrSlice4D,
			Lexi::AlignLeft, Lexi::AlignTop ) );
	}else // if d_spec4D.isNull()
	{
		d_slices[ DimZ ].d_spec4D = 0;
        d_slices[ DimW ].d_spec4D = 0;
		d_ortho.d_spec = 0;
		d_slices[ DimZ ].d_buf4D->setSpectrum( 0 );
        d_slices[ DimW ].d_buf4D->setSpectrum( 0 );
        d_ortho.d_viewer->getViews()->replace( LABEL1, 0 );
		d_mdl4D->setSpec( 0 );
		d_src4D->setSpecType( 0 );
	}

    sync4dZwSliceToCur();
	d_ortho.d_buf->setSpectrum( d_ortho.d_spec );
	syncStripWidth();

	setShow4dPlane( d_show4DPlane );
	selectCurSystem( true );
	Lexi::Viewport::popCursor();
	SpecChanged msg( true );
	traverseUp( msg );
}

void FourDScopeAgent::stepSpec2D(bool next)
{
	if( d_sort2D.size() < 2 )
		return;
	Sort::const_iterator p = d_sort2D.end();
	if( d_spec2D )
		p = d_sort2D.find( d_spec2D->getName() );
	if( p == d_sort2D.end() )
	{
		if( next )
			p = d_sort2D.begin();
		else
			--p;
	}else
	{
		if( next )
		{
			++p;
			if( p == d_sort2D.end() )
				p = d_sort2D.begin();
		}else if( p == d_sort2D.begin() )
		{
			p = d_sort2D.end();
			--p;
		}else
			--p;
	}
	assert( p != d_sort2D.end() );
	setSpec2D( (*p).second );
}

void FourDScopeAgent::stepSpec4D(bool next)
{
	if( d_sort4D.size() < 2 )
		return;
	Sort::const_iterator p = d_sort4D.end();
	if( d_spec4D )
		p = d_sort4D.find( d_spec4D->getName() );
	if( p == d_sort4D.end() )
	{
		if( next )
			p = d_sort4D.begin();
		else
			--p;
	}else
	{
		if( next )
		{
			++p;
			if( p == d_sort4D.end() )
				p = d_sort4D.begin();
		}else if( p == d_sort4D.begin() )
		{
			p = d_sort4D.end();
			--p;
		}else
			--p;
	}
	assert( p != d_sort4D.end() );
	setSpec4D( (*p).second );
}

void FourDScopeAgent::setSpec2D(Spectrum * spec )
{
    assert( spec == 0 || spec->getDimCount() == 2 );

    if( d_spec2D == spec )
		return;

	Lexi::Viewport::pushHourglass();
	d_spec2D = spec;

    d_mdl2D->setSpec( 0 );
	d_src2D->setSpec( d_spec2D );
	d_mdl2D->setSpec( d_spec2D );

    setShow4dPlane( false );

    if( d_spec2D )
        d_slices[ DimX ].d_spec2D = new SpecProjector( d_spec2D, DimX );
    else
        d_slices[ DimX ].d_spec2D = 0;
	d_slices[ DimX ].d_buf2D->setSpectrum( d_slices[ DimX ].d_spec2D );
	d_slices[ DimX ].d_buf2D->setFolding( d_folding );

    if( d_spec2D )
        d_slices[ DimY ].d_spec2D = new SpecProjector( d_spec2D, DimY );
    else
        d_slices[ DimY ].d_spec2D = 0;
	d_slices[ DimY ].d_buf2D->setSpectrum( d_slices[ DimY ].d_spec2D );
	d_slices[ DimY ].d_buf2D->setFolding( d_folding );

	Lexi::Viewport::popCursor();
}

void FourDScopeAgent::setPeakList(PeakList * pl)
{
	if( pl )
	{
		d_pl = new PeakListPeer( pl );
		d_plane.d_pp->setPeakSpace( d_pl );
        d_ortho.d_pp->setPeakSpace( d_pl );
        QString str;
		str.sprintf( "%dD %s", d_pl->getDimCount(), pl->getName().data() );
		d_plane.d_viewer->getViews()->replace( LABEL3,
			new Lexi::Label( str, nil, 
			d_plane.d_peaks->getColor(), 
			Lexi::AlignLeft, Lexi::AlignBottom ) );
	}else
	{
		d_pl = 0;
		d_plane.d_pp->setPeakSpace( new PeakSpaceDummy( s_defDim ) );
		d_ortho.d_pp->setPeakSpace( new PeakSpaceDummy( s_defDim ) );
		d_plane.d_viewer->getViews()->replace( LABEL3, 0 );
	}
}

void FourDScopeAgent::extendSystem(Dimension source, Dimension target )
{
	// Ausser spec2D identisch mit HomoScope
	Spin* ref = 0;
	if( d_ref[ source ] != 0 )
		ref = d_ref[ source ];
	else if( d_plane.d_tuples->getSel().size() == 1 )
		ref = ( *d_plane.d_tuples->getSel().begin() )[ source ];
	else
	{
		// Der User kann Extend auch ausfhren, wenn kein Peak selektiert wurde.
		// In diesem Fall schlagen wir Peaks in der Region der Cursordimension vor.
		ProposeSpinDlg dlg( MW, d_pro, getSpec()->getColor( source ), d_cursor[ source ],
                            getSpec(),	"Select Reference Spin" );
		dlg.setAnchor( source, ref );
		if( !dlg.exec() || dlg.getSpin() == 0 )
			return;
		ref = dlg.getSpin();
	}
	pickSpin( target, ref, ref->getSystem() );
}

void FourDScopeAgent::pickSpin(Dimension d, Spin *other, SpinSystem *owner)
{
	SpinLabel l = d_spec2D->getKeyLabel( d );

	// 26.6.05: immer anzeigen if( !d_src2D->showNulls() )
	{
		SpinLabelSet ly = d_spec2D->getType()->getLabels( d_spec2D->mapToType( d ) );
			// Ich lasse das vorerst so, da nicht sicher ist, ob Inference Keys enth�t.

		NmrExperiment* e = d_pro->getRepository()->getTypes()->
			inferExperiment2( d_spec2D->getType(), owner, d_spec2D );
		if( e )
			e->getColumn( d_spec2D->mapToType( d ), ly, owner );
		if( owner )
			ly = owner->getAcceptables( ly );

		if( !Dlg::getLabel( MW, l, ly ) )
			return;
		if( owner && !owner->isAcceptable( l ) )
		{
			Root::ReportToUser::alert( this, "Pick Label", "Label is not acceptable" );
			return;
		}
	}

	Root::Ref<PickSystemSpinLabelCmd> cmd = new PickSystemSpinLabelCmd( d_pro->getSpins(), 
		owner, d_spec2D->getColor( d ), d_cursor[ d ], l, 0 ); 
	if( cmd->handle( this ) )
	{
		if( l.isNull() )
			d_src2D->showNulls( true );
		d_plane.d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimY ] );
	}
}

void FourDScopeAgent::gotoTuple(SpinSystem * sys, Spin * spin, Spin * link, bool twoD )
{
	SpinPoint tuple;
	Dimension dim;
	SpinSpace::Result tuples;
	if( !twoD && d_spec4D )
	{
		if( link )
			d_mdl4D->find( tuples, spin, link );
		else if( spin )
			d_mdl4D->find( tuples, spin );
		else
			d_mdl4D->find( tuples, sys );
		dim = 4;
	}else
	{
		if( link )
			d_mdl2D->find( tuples, spin, link );
		else if( spin )
			d_mdl2D->find( tuples, spin );
		else
			d_mdl2D->find( tuples, sys );
		dim = 2;
	}
	if( tuples.empty() )
	{
		Lexi::ShowStatusMessage msg( "Goto peak: element not found in inferred base" );
		traverseUp( msg );
		return;
	}
	if( tuples.hasOne() )
	{
		tuple = tuples.first().d_point;
	}else
	{
		tuple = Dlg::selectTuple( MW, tuples, 
			dim, "Select Spin Tuple" );
		if( tuple.isZero() )
			return;
	}
	PpmPoint orig;
	for( Dimension d = 0; d < dim; d++ )
		orig.push_back( tuple[ d ]->getShift(  
			( dim == 4 )?d_spec4D:d_spec2D ) );
	bool ac = d_autoCenter;
	d_autoCenter = false;
	d_plane.d_cur->setCursor( orig[ DimX ], orig[ DimY ] );
	d_plane.d_tuples->select( tuple );
	if( dim == 4 )
	{
		d_cursor[ DimZ ] = orig[ DimZ ];
        d_cursor[ DimW ] = orig[ DimW ];
        d_slices[ DimW ].d_cur->setCursor( (Dimension)DimX, d_cursor[ DimW ] );
        d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
        d_ortho.d_cur->setCursor(d_cursor[ DimW ], d_cursor[ DimZ ]);
		sync4dXySliceToCur(true);
        sync4dZwSliceToCur();
        syncOrthoToCur();
	}
	selectCurSystem();
	centerToCursor();
	if( dim == 4 )
	{
		ViewAreaMdl* area = d_ortho.d_viewer->getViewArea();
		if( !area->getRange( DimY ).contains( d_cursor[ DimZ ] ) ||
            !area->getRange( DimX ).contains( d_cursor[ DimW ] ) )
		{
			area->centerPoint( d_cursor[ DimW ], d_cursor[ DimZ ] );
			d_ortho.d_viewer->damageMe();
		}
	}
	notifyCursor( dim == 2 );
	d_autoCenter = ac;
}

Spin* FourDScopeAgent::getSel(bool hori) const
{
	// Kopie von HomoScope
	if( d_plane.d_tuples->getSel().size() != 1 )
		return 0;

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	if( hori )
		return tuple[ DimX ];
	else
		return tuple[ DimY ];
}

void FourDScopeAgent::notifyCursor(bool plane)
{
	QString str;
    QTextStream ts( &str, QIODevice::WriteOnly );

	ts.setf( QTextStream::fixed );
	ts.precision( 3 ); // drei Stellen

	ts <<  "Cursor:  ";

    Spectrum* spec = getSpec();
    if( !plane && d_spec4D )
		spec = d_spec4D;

    for( Dimension d = 0; spec && d < spec->getDimCount(); d++ )
	{
        ts << Spec::getDimSymbolLetter( d, true ) << ": ";
		ts << spec->getDimName( d ) << "=";	// wegen Fred
		ts << d_cursor[ d ];
		if( d_folding )
			ts << " (" << spec->getScale( d ).getRangeOffset( d_cursor[ d ] ) << ")  ";
		else
			ts << "  ";
    }

	try
	{
		Amplitude val = 0;
        if( plane || d_spec4D.isNull() )
		{
			PpmPoint p;
			for( Dimension d = 0; d < getSpec()->getDimCount(); d++ )
				p.push_back( d_cursor[ d ] );
			val = getSpec()->getAt( p, d_folding, d_folding );
		}else 
			val = d_spec4D->getAt( d_cursor, d_folding, d_folding );
		ts.setf( QTextStream::showpos );
		ts.precision( 0 );
		ts << "Level=" << val;
	}catch( ... )
	{
		ts << " Out of Spectrum";
	}
	QByteArray  tmp;
	SpinPointView* tv = 0;
	if( plane )
		tv = d_plane.d_tuples;
	else if( d_ortho.d_viewer->hasFocus() )
		tv = d_ortho.d_tuples;
	if( tv && tv->formatSelection( tmp, SpinPointView::PairAll, 3 ) )
	{
		str += ",  ";
		str += tmp.data();
	}
	Lexi::ShowStatusMessage msg( str );
	traverseUp( msg );
}

void FourDScopeAgent::sync4dZwSliceToCur()
{
    sync4dZwSliceToCur( DimZ );
    sync4dZwSliceToCur( DimW );
}

void FourDScopeAgent::sync4dZwSliceToCur(Dimension dim)
{
	if( d_spec4D.isNull() )
		return;

    if( d_slices[ dim ].d_spec4D )
        d_slices[ dim ].d_spec4D->setOrigin( d_cursor );
	d_slices[ dim ].d_viewer->redraw();
}

#ifdef __unused
static void _allocate( PpmRange& r, PPM point, PPM width ) // TODO: Bug Release-Mode
{
	if( r.first <= r.second )
	{
		r.first = point - width / 2.0;
		r.second = point + width / 2.0;
	}else
	{
		r.first = point + width / 2.0;
		r.second = point - width / 2.0;
	}
}
#endif

void FourDScopeAgent::syncStripWidth()
{
	if( d_spec4D.isNull() )
		return;
//	PpmRange rx = getSpec()->getScale( DimX ).getRange();
//	PPM wx = d_pro->inferStripWidth( DimX, d_spec4D );
//	_allocate( rx, d_cursor[ DimX ], wx );
//    PpmRange ry = getSpec()->getScale( DimY ).getRange();
//	PPM wy = d_pro->inferStripWidth( DimY, d_spec4D );
//	_allocate( ry, d_cursor[ DimY ], wy );
//	d_ortho.d_viewer->getViewArea()->setRange( rx, ry );

	PPM wx = d_pro->inferPeakWidth( DimX, d_spec4D );
    PPM wy = d_pro->inferPeakWidth( DimY, d_spec4D );
	d_ortho.d_mdl->setGhostWidth( wx, wy );
	d_ortho.d_pp->setDimGhostWidth( DimX, DimY, wx, wy );

	d_ortho.d_viewer->redraw();
}

void FourDScopeAgent::syncOrthoToCur()
{
	if( d_spec4D.isNull() )
		return;
	d_lock = true;

	d_ortho.d_spec->setOrigin( d_cursor );

	d_ortho.d_mdl->setOrigThick( d_cursor[ DimX ], d_cursor[ DimY ],
                             d_spec4D->getScale( DimX ).getDelta(),
                             d_spec4D->getScale( DimY ).getDelta(), true );	// RISK
    d_ortho.d_pp->setOrigThick( DimX, d_cursor[ DimX ],
		d_spec4D->getScale( DimX ).getDelta() );
	d_ortho.d_pp->setOrigThick( DimY, d_cursor[ DimY ],
		d_spec4D->getScale( DimY ).getDelta() );

	syncStripWidth();
	selectCurSystem();

	d_lock = false;
}

void FourDScopeAgent::sync4dXySliceToCur( Dimension dim, bool show )
{
	assert( dim == DimX || dim == DimY );

	if( ( !d_autoHide || show || d_show4DPlane ) && !d_spec4D.isNull() )
	{
        if( d_slices[ dim ].d_spec4D.isNull() )
            d_slices[ dim ].d_spec4D = new SpecProjector( d_spec4D, dim );
        d_slices[ dim ].d_spec4D->setOrigin( d_cursor );
	}else
	{
		d_slices[ dim ].d_spec4D = 0;
	}
	d_slices[ dim ].d_buf4D->setSpectrum( d_slices[ dim ].d_spec4D );
	d_slices[ dim ].d_viewer->redraw();
}

void FourDScopeAgent::updateContour4D( bool ac )
{
	if( ac )
	{
		d_ortho.d_view->createLevelsAuto();
	}else
		d_ortho.d_view->createLevelsMin( (d_spec4D)?d_spec4D->getThreshold():0.0 );
	d_ortho.d_viewer->damageMe();
}

void FourDScopeAgent::updateRef()
{
	if( !d_ref.isZero() )
	{
		char buf[ 64 ];
		d_plane.d_tuples->formatLabel( buf, sizeof( buf ), d_ref );
		QString str;
		str.sprintf( "Reference: %s", buf );
		d_plane.d_viewer->getViews()->replace( LABEL2,
			new Lexi::Label( str, nil, g_clrLabel, 
				Lexi::AlignRight, Lexi::AlignTop ) );
	}else
		d_plane.d_viewer->getViews()->replace( LABEL2, 0 );
	d_plane.d_viewer->redraw();
}

void FourDScopeAgent::selectCurSystem( bool force )
{
	if( d_spec4D && ( d_plane.d_tuples->getSel().size() == 1 ||
		!d_ref.isZero() ) )
	{
		SpinPoint tuple;
		if( !d_ref.isZero() )
			tuple = d_ref;
		else
			tuple = *d_plane.d_tuples->getSel().begin();
		if( !force && d_cur == tuple )
			return; // Bereits korrekt
		d_cur = tuple;

		QString str;
        char buf[32];
        SpinPointView::formatLabel( buf, sizeof(buf), d_cur,
                                    SpinPointView::PairIdLabelSysOrResi, DimUndefined );
        str = buf;
        d_ortho.d_viewer->getViews()->replace( LABEL1,
			new Lexi::Label( str, nil, g_clrSlice4D,
			Lexi::AlignLeft, Lexi::AlignTop ) );

        str.sprintf( " %s/%s", d_spec4D->getDimName( DimW ),
                     d_spec4D->getDimName( DimZ ) );

		d_ortho.d_viewer->getViews()->replace( LABEL2,
			new Lexi::Label( str, nil, g_clrSlice4D,
			Lexi::AlignLeft, Lexi::AlignBottom ) );

		d_ortho.d_mdl->setSys( d_cur[ DimX ]->getSystem() );
	}else
	{
		d_cur.zero();
        d_ortho.d_viewer->getViews()->replace( LABEL2, 0 );
        d_ortho.d_viewer->getViews()->replace( LABEL1, 0 );
        d_ortho.d_mdl->setSys( 0 );
	}
}

void FourDScopeAgent::handleSetResolution(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	bool ok	= FALSE;
	int res	= QInputDialog::getInteger( "Set Resolution", 
		"Please	enter the minimal number of pixels per sample:", 
		d_resol, 1, 20, 1,	&ok, MW );
	if( ok )
	{
		d_resol = res;
		d_lowResol = true;
		Viewport::pushHourglass();
		d_plane.d_ol[0].d_buf->setResolution( d_resol );
		d_plane.d_viewer->damageMe();
		Viewport::popCursor();
	}
}

void FourDScopeAgent::handleShowLowRes(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_lowResol );

	Viewport::pushHourglass();
	d_lowResol = !d_lowResol;
	if( d_lowResol )
		d_plane.d_ol[0].d_buf->setResolution( d_resol );
	else
		d_plane.d_ol[0].d_buf->setScaling( false );
	d_plane.d_viewer->damageMe();
	Viewport::popCursor();
}

void FourDScopeAgent::handleForward(Action & a)
{
	return;
	ACTION_ENABLED_IF( a, false ); // TODO d_forward.size()	> 0 );

	Viewport::pushHourglass();
	d_backward.push_back( d_forward.back() );
	const PpmCube& cube = d_forward.back().first;
	d_cursor = d_backward.back().second;
	d_plane.d_viewer->getViewArea()->setRange( DimX, cube[ DimX ] );
	d_plane.d_viewer->getViewArea()->setRange( DimY, cube[ DimY ] );
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
	for( Dimension d = 0; d < d_slices.size(); ++d )
	{
		d_slices[ d ].d_cur->setCursor( (Dimension)DimY, d_cursor[ d ] );
		d_slices[ d ].d_viewer->getViewArea()->setRange( 
			d_slices[ d ].d_viewer->getViewArea()->getDim(), cube[ d ] );
		d_slices[ d ].d_viewer->redraw();
	}
	d_forward.pop_back();
	d_plane.d_viewer->damageMe();
	Viewport::popCursor();
}

void FourDScopeAgent::handleBackward(Action & a)
{
	return;
	ACTION_ENABLED_IF( a, false ); // TODO d_backward.size() > 1 );

	Viewport::pushHourglass();
	d_forward.push_back( d_backward.back() );
	d_backward.pop_back();
	const PpmCube& cube = d_backward.back().first;
	d_cursor = d_forward.back().second;
	d_plane.d_viewer->getViewArea()->setRange( DimX, cube[ DimX ] );
	d_plane.d_viewer->getViewArea()->setRange( DimY, cube[ DimY ] );
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
	for( Dimension d = 0; d < d_slices.size(); ++d )
	{
		d_slices[ d ].d_cur->setCursor( (Dimension)DimY, d_cursor[ d ] );
		d_slices[ d ].d_viewer->getViewArea()->setRange( 
			d_slices[ d ].d_viewer->getViewArea()->getDim(), cube[ d ] );
		d_slices[ d ].d_viewer->redraw();
	}
	d_backward.pop_back();
	d_plane.d_viewer->damageMe();
	Viewport::popCursor();
}

void FourDScopeAgent::handleFitWindow(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	d_plane.d_ol[0].d_buf->fitToArea();
	/*
	d_autoContour = true;	// Wegen Pascal
	updateContour( 0, true );
	*/
	d_plane.d_viewer->damageMe();
	for( Dimension d = 0; d < d_slices.size(); ++d )
	{
		d_slices[ d ].d_buf2D->fitToArea();
		d_slices[ d ].d_viewer->redraw();
	}
}

void FourDScopeAgent::handleShowFolded(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_folding );

	Viewport::pushHourglass();
	d_folding = !d_folding;
	for( int i = 0; i < d_plane.d_ol.size(); i++ )
		d_plane.d_ol[i].d_buf->setFolding( d_folding );
	if( d_folding )
		d_plane.d_viewer->getViews()->replace( FOLDING, new FoldingView( d_plane.d_ol[0].d_buf ) );
	else
		d_plane.d_viewer->getViews()->replace( FOLDING, 0 );
	d_plane.d_viewer->damageMe();
	Dimension d;
	for( d = 0; d < d_slices.size(); ++d )
	{
		d_slices[ d ].d_buf2D->setFolding( d_folding );
		d_slices[ d ].d_buf4D->setFolding( d_folding );
		d_slices[ d ].d_viewer->redraw();
	}
    d_ortho.d_buf->setFolding( d_folding );
    if( d_folding )
        d_ortho.d_viewer->getViews()->replace( FOLDING, new FoldingView( d_ortho.d_buf ) );
    else
        d_ortho.d_viewer->getViews()->replace( FOLDING, 0 );
    d_ortho.d_viewer->redraw();
    Viewport::popCursor();
}

void FourDScopeAgent::handleSpecCalibrate(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	Spectrum* spec = d_spec2D;
	if( d_spec4D && d_show4DPlane )
		spec = d_spec4D;

	PpmPoint p( 0, 0 );
	for( Dimension d = 0; d < 2; d++ )
		p[ d ] = tuple[ d ]->getShift( spec ) - d_cursor[ d ];

	Viewport::pushHourglass();
	Root::Ref<SpecCalibrateCmd> cmd = new SpecCalibrateCmd( spec, p );
	cmd->handle( this );
	Viewport::popCursor();
}

void FourDScopeAgent::handleAutoCenter(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_autoCenter );

	d_autoCenter = !d_autoCenter;
}

void FourDScopeAgent::handleShowContour(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_plane.d_ol[0].d_view->isVisi() );

	bool visi = !d_plane.d_ol[0].d_view->isVisi();
	for( int i = 0; i < d_plane.d_ol.size(); i++ )
		d_plane.d_ol[i].d_view->setVisi( visi );
	if( d_plane.d_ol[0].d_view->isVisi() )
		d_plane.d_viewer->getViews()->replace( BACKGROUND, new Lexi::Background() );
	else
		d_plane.d_viewer->getViews()->replace( BACKGROUND, 0 );
	updateContour( 0, true );
}

void FourDScopeAgent::handleShowIntensity(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_plane.d_intens->isVisi() );

	showIntens( !d_plane.d_intens->isVisi() );
}

void FourDScopeAgent::handleAutoContour(Action & a)
{
    ACTION_CHECKED_IF( a, ( d_show4DPlane && d_aol == 0 ) || d_plane.d_ol[d_aol].d_spec,
        ( d_show4DPlane && d_ortho.d_view->isAuto() ) ||
        ( !d_show4DPlane && d_plane.d_ol[d_aol].d_view->isAuto() ) );
	
	if( d_show4DPlane && d_aol == 0 )
		handleAutoContour2( a );
	else
	{
		if( d_plane.d_ol[d_aol].d_view->isAuto() )
			d_plane.d_ol[d_aol].d_view->createLevelsMin( d_plane.d_ol[d_aol].d_spec->getThreshold() );
		else
			d_plane.d_ol[d_aol].d_view->createLevelsAuto();
		d_plane.d_viewer->redraw();
	}
}

void FourDScopeAgent::handleContourParams(Action & a)
{
	ACTION_ENABLED_IF( a, d_show4DPlane && d_aol == 0 || d_plane.d_ol[d_aol].d_spec );

	if( d_show4DPlane && d_aol == 0 )
	{
		handleContourParams2( a );
		return;
	}
	Dlg::ContourParams p;
	p.d_factor = d_plane.d_ol[d_aol].d_view->getFactor();
	p.d_threshold =	d_plane.d_ol[d_aol].d_spec->getThreshold();
	p.d_option = d_plane.d_ol[d_aol].d_view->getOption();
	if( Dlg::setParams( MW, p ) )
	{
		d_plane.d_ol[d_aol].d_spec->setThreshold( p.d_threshold );
		d_plane.d_ol[d_aol].d_view->setOption( p.d_option );
		d_plane.d_ol[d_aol].d_view->setFactor( p.d_factor );
		d_plane.d_ol[d_aol].d_view->setVisi( true );
		showIntens( false );
		d_plane.d_ol[d_aol].d_view->createLevelsMin( d_plane.d_ol[d_aol].d_spec->getThreshold() );
		d_plane.d_viewer->damageMe();
	}
}

void FourDScopeAgent::handlePickSystem(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	PpmPoint xyPoint( d_cursor[ DimX ], d_cursor[ DimY ] );
	bool ok = true;
	if( !d_spec2D.isNull() && !d_show4DPlane )
	{
        // 2D
        Dlg::LP lp;
        lp.d_x = d_spec2D->getKeyLabel( DimX );
        lp.d_y = d_spec2D->getKeyLabel( DimY );

        if( !Dlg::getLabelsSysType( MW, lp, d_pro->getRepository(), d_spec2D->getType(),
                                    d_spec2D->mapToType( DimX ), d_spec2D->mapToType( DimY ) ) )
            return;
        Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick System" );
        Root::Ref<CreateSystemPairCmd> c1 =
                new CreateSystemPairCmd( d_pro->getSpins(),	xyPoint, d_spec2D, lp.d_sys );
        c1->execute();
        cmd->add( c1 );
        if( !c1->getSpin( DimX )->getLabel().equals( lp.d_x ) )
            cmd->add( new LabelSpinCmd( d_pro->getSpins(), c1->getSpin( DimX ), lp.d_x ) );
        if( !c1->getSpin( DimY )->getLabel().equals( lp.d_y ) )
            cmd->add( new LabelSpinCmd( d_pro->getSpins(), c1->getSpin( DimY ), lp.d_y ) );
        ok = cmd->handle( this );
        if( lp.d_x.isNull() || lp.d_y.isNull() )
            d_src2D->showNulls( true );
    }else // if( d_spec2D.isNull() || d_show4DPlane )
	{
        // 4D
        SpinLabelPoints pairs;
        pairs.append(SpinLabelPoint());
		NmrExperiment* e = d_pro->getRepository()->getTypes()->inferExperiment3( d_spec4D->getType(), true );
        if( e )
        {
            pairs = e->getQuadruples( PathTable::Path(), d_spec4D->mapToType( DimX ), d_spec4D->mapToType( DimY ),
                                      d_spec4D->mapToType( DimZ ), d_spec4D->mapToType( DimW ) );
        }
        SpinLabelPoint pair;
        if( !Dlg::getLabelPoint( MW, pairs, pair, 4, "Pick System" ) )
            return;
        Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick System" );
        Root::Ref<CreateSystemPairCmd> c1 = new CreateSystemPairCmd( d_pro->getSpins(),	xyPoint, d_spec4D );
        c1->execute();
        cmd->add( c1 );
        if( !c1->getSpin( DimX )->getLabel().equals( pair[DimX] ) )
            cmd->add( new LabelSpinCmd( d_pro->getSpins(), c1->getSpin( DimX ), pair[DimX] ) );
        if( !c1->getSpin( DimY )->getLabel().equals( pair[DimY] ) )
            cmd->add( new LabelSpinCmd( d_pro->getSpins(), c1->getSpin( DimY ), pair[DimY] ) );
        cmd->add( new PickSystemSpinLabelCmd( d_pro->getSpins(), c1->getSystem(),
                                              d_spec4D->getColor( DimZ ), d_cursor[ DimZ ], pair[DimZ], 0 ) );
        cmd->add( new PickSystemSpinLabelCmd( d_pro->getSpins(), c1->getSystem(),
                                              d_spec4D->getColor( DimW ), d_cursor[ DimW ], pair[DimW], 0 ) );
        ok = cmd->handle( this );
    }
	if( ok )
	{
		d_plane.d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimY ] );
		if( d_autoHold && !d_plane.d_tuples->getSel().empty() )
			d_ref = *d_plane.d_tuples->getSel().begin();
		updateRef();
		selectCurSystem();
        d_ortho.d_tuples->selectPeak( d_cursor[ DimW ], d_cursor[ DimZ ] );
    }
}

void FourDScopeAgent::handlePickHori(Action & a)
{
    ACTION_ENABLED_IF( a, !d_show4DPlane && !d_spec2D.isNull() || d_show4DPlane && !d_spec4D.isNull() );
    if( !d_show4DPlane )
        extendSystem( DimY, DimX );
    else
        extendSystem4D( DimY, DimX, true );
}

void FourDScopeAgent::extendSystem4D( Dimension main, Dimension ortho, bool plane )
{
    Q_ASSERT( d_spec4D );
    Spin* ref = 0;
    const Dimension dim = ( ortho== DimY || ortho == DimZ )?DimY:DimX;
    if( d_ref[ ortho ] != 0 )
        ref = d_ref[ ortho ];
    else if( !plane && d_ortho.d_tuples->getSel().size() == 1 )
        ref = ( *d_ortho.d_tuples->getSel().begin() )[ dim ];
    else if( plane && d_plane.d_tuples->getSel().size() == 1 )
        ref = ( *d_plane.d_tuples->getSel().begin() )[ dim ];
    else
    {
        // Der User kann Extend auch ausfhren, wenn kein Peak selektiert wurde.
        // In diesem Fall schlagen wir Peaks in der Region der Cursordimension vor.
        ProposeSpinDlg dlg( MW, d_pro, d_spec4D->getColor( ortho ), d_cursor[ ortho ],
                            d_spec4D,	"Select Reference Spin" );
        dlg.setAnchor( ortho, ref );
        if( !dlg.exec() || dlg.getSpin() == 0 )
            return;
        ref = dlg.getSpin();
    }
    SpinLabelSet ly = d_spec4D->getType()->getLabels( d_spec4D->mapToType( main ) );
    NmrExperiment* e = d_pro->getRepository()->getTypes()->
			inferExperiment2( d_spec4D->getType(), ref->getSystem(), d_spec4D );
    if( e )
        e->getColumn( d_spec4D->mapToType( main ), ly, ref->getSystem() );
    if( ref->getSystem() )
        ly = ref->getSystem()->getAcceptables( ly );

    SpinLabel l = d_spec4D->getKeyLabel( main );
    if( !Dlg::getLabel( MW, l, ly ) )
			return;
    if( ref->getSystem() && !ref->getSystem()->isAcceptable( l ) )
    {
        Root::ReportToUser::alert( this, "Pick Spin", "Label is not acceptable" );
        return;
    }

	Root::Ref<PickSystemSpinLabelCmd> cmd = new PickSystemSpinLabelCmd( d_pro->getSpins(),
		ref->getSystem(), d_spec4D->getColor( main ), d_cursor[ main ], l, 0 );
	if( cmd->handle( this ) )
	{
		if( l.isNull() )
			d_src4D->showNulls( true );
		d_ortho.d_tuples->selectPeak( d_cursor[ DimW ], d_cursor[ DimZ ] );
	}
}

void FourDScopeAgent::handlePickHori4D(Action & a)
{
	ACTION_ENABLED_IF( a, d_spec4D );
    extendSystem4D( DimW, DimZ, false );
}

void FourDScopeAgent::handlePickVerti4D(Action & a)
{
	ACTION_ENABLED_IF( a, d_spec4D );
    extendSystem4D( DimZ, DimW, false );
}

void FourDScopeAgent::handlePickVerti(Action & a)
{
    ACTION_ENABLED_IF( a, !d_show4DPlane && !d_spec2D.isNull() || d_show4DPlane && !d_spec4D.isNull() );
    if( !d_show4DPlane )
        extendSystem( DimX, DimY );
    else
        extendSystem4D( DimX, DimY, true );
}

void FourDScopeAgent::handleMovePeak(Action & a)
{
    // Verschiebt nur den 2D-Tuple
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spin System" );
	for( Dimension d = 0; d < 2; d++ )
	{
		cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ d ], 
			d_cursor[ d ], 0 ) ); // Move generisches Spektrum
	}
	cmd->handle( this );
	selectCurSystem( true );
}

void FourDScopeAgent::handleMovePeakAlias(Action & a)
{
    // Verschiebt nur den 2D-Tuple
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spin System" );
    Spectrum* spec = getSpec();
	for( Dimension d = 0; d < 2; d++ )
	{
		if( d_cursor[ d ] != tuple[ d ]->getShift( spec ) )
			cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ d ], d_cursor[ d ], spec ) );
	}
	cmd->handle( this );
	selectCurSystem( true );
}

void FourDScopeAgent::handleLabelPeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

    Spectrum* spec = getSpec();
	SpinLabel x = tuple[ DimX ]->getLabel();
	SpinLabel y = tuple[ DimY ]->getLabel();
	SpinLabelSet lx = spec->getType()->getLabels( spec->mapToType( DimX ) );
	SpinLabelSet ly = spec->getType()->getLabels( spec->mapToType( DimY ) );
		// lx/y soll vorerst alle statischen Labels auch sehen. Wenn man das wegl�st,
		// verschwinden zur Zeit noch CB aus der Liste von HNCACB von Freds Template.
		// Zu grosse Auswirkungen auf bestehende Projekte.
	NmrExperiment* e;
	e = d_pro->getRepository()->getTypes()->
		inferExperiment2( spec->getType(), tuple[ DimX ]->getSystem(), spec );
	if( e )
		e->getColumn( spec->mapToType( DimX ), lx );
	if( tuple[ DimX ]->getSystem() )
		lx = tuple[ DimX ]->getSystem()->getAcceptables( lx );
	e = d_pro->getRepository()->getTypes()->
		inferExperiment2( spec->getType(), tuple[ DimY ]->getSystem(), spec );
	if( e )
		e->getColumn( spec->mapToType( DimY ), ly );
	if( tuple[ DimY ]->getSystem() )
		ly = tuple[ DimY ]->getSystem()->getAcceptables( ly );
	if( !Dlg::getLabels( MW, tuple[ DimX ]->getId(), tuple[ DimY ]->getId(), x, y, lx, ly ) )
		return;

	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Label Peak" );
	if( !tuple[ DimX ]->getLabel().equals( x ) )
		cmd->add( new LabelSpinCmd( d_pro->getSpins(), tuple[ DimX ], x ) ); 
	if( !tuple[ DimY ]->getLabel().equals( y ) )
		cmd->add( new LabelSpinCmd( d_pro->getSpins(), tuple[ DimY ], y ) ); 
	if( !cmd->empty() )
		cmd->handle( this );
	if( x.isNull() || y.isNull() )
		d_src2D->showNulls( true );
}

void FourDScopeAgent::handleHidePeak(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinLink* link = tuple[ DimX ]->findLink( tuple[ DimY ] );
	ACTION_ENABLED_IF( a, link );
	Root::Ref<HideSpinLinkCmd> cmd = new HideSpinLinkCmd( d_pro->getSpins(), link, getSpec() );
	cmd->handle( this );
	// TODO: Plural
}

void FourDScopeAgent::handleDeletePeak(Action & a)
{
	ACTION_ENABLED_IF( a, !d_plane.d_tuples->getSel().empty() );

	SpinPointView::Selection sel = d_plane.d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
	SpinBase* base = d_pro->getSpins();
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Spins" );
	std::set<Spin*> test;
	for( p = sel.begin(); p != sel.end(); ++p )
	{
		if( test.count( (*p)[ DimX ] ) == 0 )
		{
			cmd->add( new DeleteSpinCmd( base, (*p)[ DimX ] ) );
			test.insert( (*p)[ DimX ] );
		}
		if( test.count( (*p)[ DimY ] ) == 0 )
		{
			cmd->add( new DeleteSpinCmd( base, (*p)[ DimY ] ) );
			test.insert( (*p)[ DimY ] );
		}
	}
	cmd->handle( this );
	selectCurSystem();
}

void FourDScopeAgent::handleDeleteSpinX(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimX ] );
	cmd->handle( this );
}

void FourDScopeAgent::handleDeleteSpinY(Action & a)
{
	ACTION_ENABLED_IF( a, d_ortho.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_ortho.d_tuples->getSel().begin();

	Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimY ] );
	cmd->handle( this );
}

void FourDScopeAgent::handleDeleteSpinX4D(Action & a)
{
	ACTION_ENABLED_IF( a, d_ortho.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_ortho.d_tuples->getSel().begin();

	Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimX ] );
	cmd->handle( this );
}

void FourDScopeAgent::handleDeleteSpinY4D(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimY ] );
	cmd->handle( this );
}

void FourDScopeAgent::handleShowAllPeaks(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_mdl2D->showAll() );
	d_mdl2D->showAll( !d_mdl2D->showAll() );
	d_mdl4D->showAll( d_mdl2D->showAll() );
}

void FourDScopeAgent::handleShowAlignment(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	ACTION_ENABLED_IF( a, tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

	SpinSystem* sys = tuple[ DimX ]->getSystem();
	
	SpinSystemString fra;
	d_pro->getSpins()->fillString( sys, fra );

	FragmentAssignment* f = new FragmentAssignment( d_pro->getSpins(),
		d_pro->getMatcher(), fra );
	SingleAlignmentView* v = new SingleAlignmentView( this, f );
	v->show();
}

void FourDScopeAgent::handleAssign(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	ACTION_ENABLED_IF( a, tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

	SpinSystem* sys = tuple[ DimX ]->getSystem();
	bool ok;
	QString str;
	str.sprintf( "Assignment for spin system %d:", sys->getId() );
	int r = QInputDialog::getInteger( "Assign Strips", 
		str, (sys->getAssig())?sys->getAssig()->getId():0, 
		-999999, 999999, 1, &ok, MW );
	if( !ok )
		return;
	Residue* res = d_pro->getSequence()->getResidue( r );
	if( res == 0 )
	{
		Root::ReportToUser::alert( this, "Assign", "Unknown residue!" );
		return;
	}

	Root::Ref<AssignSystemCmd> cmd =
		new AssignSystemCmd( d_pro->getSpins(), sys, res );
	cmd->handle( this );
}

void FourDScopeAgent::handleUnassign(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	ACTION_ENABLED_IF( a, tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

	SpinSystem* sys = tuple[ DimX ]->getSystem();

	Root::Ref<UnassignSystemCmd> cmd =
		new UnassignSystemCmd( d_pro->getSpins(), sys );
	cmd->handle( this );
}

void FourDScopeAgent::handleSetSystemType(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	ACTION_ENABLED_IF( a, tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

	SpinSystem* sys = tuple[ DimX ]->getSystem();

	Repository::SystemTypeMap::const_iterator p;
	const Repository::SystemTypeMap& sm = d_pro->getRepository()->getSystemTypes();

	int cur = 0;
	QStringList l;
	l.append( "" );
	for( p = sm.begin(); p != sm.end(); ++p )
		l.append( (*p).second->getName().data() ); // NOTE: Name sollte Unique sein.
	l.sort();
	QString tn;
	if( sys->getSysType() )
        tn = sys->getSysType()->getName().data();
	for( int i = 0; i < l.count(); i++ )
		if( tn == l[ i ] )
		{
			cur = i;
			break;
		}
	bool ok;
	QString res = QInputDialog::getItem( "Set System Type", "Select a spin system type:", 
		l, cur, false, &ok, MW );
	if( !ok )
		return;

	SystemType* sst = 0;
	for( p = sm.begin(); p != sm.end(); ++p )
	{
		if( res == (*p).second->getName().data() )
		{
			sst = (*p).second;
			break;
		}
	}

	Root::Ref<ClassifySystemCmd> cmd =
		new ClassifySystemCmd( d_pro->getSpins(), sys, sst );
	cmd->handle( this );
}

void FourDScopeAgent::handleViewLabels(Action & a)
{
	if( a.getParamCount() == 0 )
		return;

	SpinPointView::Label q = (SpinPointView::Label) a.getParam( 0 ).getShort();
	if( q < SpinPointView::None || q >= SpinPointView::End )
		return;

	ACTION_CHECKED_IF( a, true,
		d_plane.d_tuples->getLabel() == q );
	
	d_plane.d_tuples->setLabel( q );
	d_plane.d_viewer->redraw();
}

void FourDScopeAgent::handleSelectSpec2D(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_spec2D == a.getParam( 0 ).getObject() );

	setSpec2D( dynamic_cast<Spectrum*>( a.getParam( 0 ).getObject() ) );
}

void FourDScopeAgent::handleLinkSystems(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	int pred, succ;
	if( !a.getParam( 0 ).isNull() && !a.getParam( 1 ).isNull() )
	{
		pred = a.getParam( 0 ).getLong();
		succ = a.getParam( 1 ).getLong();
	}else if( !Dlg::getPredSucc( MW, pred, succ ) )
		return;

	SpinSystem* p = d_pro->getSpins()->getSystem( pred );
	SpinSystem* s = d_pro->getSpins()->getSystem( succ );
	if( p == 0 || s == 0 )
	{
		Root::ReportToUser::alert( this, "Link Systems", "Unknown Systems" );
		return;
	}

	try
	{
		Root::Ref<LinkSystemCmd> cmd =
			new LinkSystemCmd( d_pro->getSpins(), p, s );
		cmd->handle( this );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Link to Reference", e.what() );
	}
}

void FourDScopeAgent::handleUnlinkPred(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	ACTION_ENABLED_IF( a, tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() &&
		tuple[ DimX ]->getSystem()->getPred() != 0 );

	SpinSystem* other = tuple[ DimX ]->getSystem()->getPred();
	Root::Ref<UnlinkSystemCmd> cmd =
		new UnlinkSystemCmd( d_pro->getSpins(), other, tuple[ DimX ]->getSystem() );
	cmd->handle( this );
}

void FourDScopeAgent::handleUnlinkSucc(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	ACTION_ENABLED_IF( a, tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() &&
		tuple[ DimX ]->getSystem()->getSucc() != 0 );

	SpinSystem* other = tuple[ DimX ]->getSystem()->getSucc();
	Root::Ref<UnlinkSystemCmd> cmd =
		new UnlinkSystemCmd( d_pro->getSpins(), tuple[ DimX ]->getSystem(), other );
	cmd->handle( this );
}

void FourDScopeAgent::handleUnhidePeak(Action & a)
{
	/*
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinLink* link = tuple[ DimX ]->findLink( tuple[ DimY ] );
	ACTION_ENABLED_IF( a, link && !link->isVisible( d_spec2D ) );
	d_pro->getSpins()->setVisible( link, true, d_spec2D ); // TODO Undo
	// TODO: Plural
	*/
}

void FourDScopeAgent::handleHoldReference(Action & a)
{
	ACTION_CHECKED_IF( a, 
		!d_ref.isZero() || d_plane.d_tuples->getSel().size() == 1, !d_ref.isZero() );
	
	if( !d_ref.isZero() && d_plane.d_tuples->getSel().empty() )
		d_ref.zero(); // Hebe Zustand auf
	else if( !d_plane.d_tuples->getSel().empty() )
	{
		d_ref = *d_plane.d_tuples->getSel().begin();
		if( d_autoRuler )
		{
			d_plane.d_hRulerMdl->removeAll();
			d_plane.d_vRulerMdl->removeAll();
			SpinPoint t;
			t[ 0 ] = d_ref[ DimY ];
			d_plane.d_hRulerMdl->addPoint( t );
			t[ 0 ] = d_ref[ DimX ];
			d_plane.d_vRulerMdl->addPoint( t );
		}
	}else
		return;

	updateRef();
	selectCurSystem();
}

void FourDScopeAgent::handleCreateReport(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	Dlg::ContourParams p;
	p.d_factor = d_plane.d_ol[0].d_view->getFactor();
	p.d_threshold =	d_spec2D->getThreshold();
	p.d_option = d_plane.d_ol[0].d_view->getOption();
	ReportViewer* rv = ReportViewer::getViewer( getParent()->getParent(), p, 
		d_plane.d_ol[0].d_view->getGain(), d_plane.d_ol[0].d_view->isAuto(), d_folding,
		(d_plane.d_ol.size()>1)?d_pro->getRepository():0 );
	ReportViewer::Spector vec;
	for( int i = 0; i < d_plane.d_ol.size(); i++ )
		if( d_plane.d_ol[i].d_spec )
			vec.push_back( d_plane.d_ol[i].d_spec );
	rv->showPlane( d_plane.d_viewer->getViewArea(), 
		vec, new PeakProjector( *d_plane.d_pp ), d_plane.d_tuples->getModel() );
}

void FourDScopeAgent::handleAddRulerVerti(Action & a)
{
	// Erzeuge horizontal verlaufender Ruler an Position des Y-Spins
	ACTION_ENABLED_IF( a, !d_plane.d_tuples->getSel().empty() );

	SpinPointView::Selection sel = d_plane.d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
	SpinPoint t;
	for( p = sel.begin(); p != sel.end(); ++ p )
	{
		t[ 0 ] = (*p)[ DimY ];
		d_plane.d_hRulerMdl->addPoint( t );
	}
}

void FourDScopeAgent::handleAddRulerHori(Action & a)
{
	// Erzeuge vertikal verlaufender Ruler an Position des X-Spins
	ACTION_ENABLED_IF( a, !d_plane.d_tuples->getSel().empty() );

	SpinPointView::Selection sel = d_plane.d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
	SpinPoint t;
	for( p = sel.begin(); p != sel.end(); ++ p )
	{
		t[ 0 ] = (*p)[ DimX ];
		d_plane.d_vRulerMdl->addPoint( t );
	}
}

void FourDScopeAgent::handleRemoveRulers(Action & a)
{
	ACTION_ENABLED_IF( a, !d_plane.d_hRuler->getSel().empty() ||
		!d_plane.d_vRuler->getSel().empty() );

	SpinPoint1DView::Selection sel = d_plane.d_hRuler->getSel();
	SpinPoint1DView::Selection::const_iterator p;
	for( p = sel.begin(); p != sel.end(); ++p )
	{
		d_plane.d_hRulerMdl->removePoint( (*p) );
	}
	sel = d_plane.d_vRuler->getSel();
	for( p = sel.begin(); p != sel.end(); ++p )
	{
		d_plane.d_vRulerMdl->removePoint( (*p) );
	}
}

void FourDScopeAgent::handleRemoveAllRulers(Action & a)
{
	ACTION_ENABLED_IF( a, !d_plane.d_hRulerMdl->isEmpty() ||
		!d_plane.d_vRulerMdl->isEmpty() );
	d_plane.d_hRulerMdl->removeAll();
	d_plane.d_vRulerMdl->removeAll();
}

void FourDScopeAgent::handleAutoRuler(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_autoRuler );

	d_autoRuler = !d_autoRuler;
}

void FourDScopeAgent::handleProposeHori(Action & a)
{
	if( d_ref.isZero() && d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = d_ref;
	if( d_ref[ DimX ] == 0 )
		tuple = *d_plane.d_tuples->getSel().begin();

	Spin* ref = tuple[ DimY ];
	ACTION_ENABLED_IF( a, !d_show4DPlane && ref->getSystem() && getSpec()->hasNoesy() );

	ProposeSpinDlg dlg( MW, d_pro, getSpec()->getColor( DimX ), d_cursor[ DimX ],
                        getSpec(), "Select Horizontal Partner" );
	dlg.setAnchor( DimY, ref );
	if( !dlg.exec() || dlg.getSpin() == 0 )
		return;

	// TODO: wenn target kein SpinSystem hat, dann ins ref einfgen.
	// TODO: wenn target und ref System haben, einen SysLink vorschlagen (Seite?)

	if( ref->findLink( dlg.getSpin() ) == 0 )	// Ref == target zul�sig wegen Diagonaler
	{
		Root::Ref<LinkSpinCmd> cmd = new LinkSpinCmd( d_pro->getSpins(), ref, dlg.getSpin() ); 
		cmd->handle( this );
	}else
		Root::ReportToUser::alert( this, "Propose Horizontal Extension", 
			"The selected spins are already linked!" );

}

void FourDScopeAgent::handleProposeVerti(Action & a)
{
	if( d_ref.isZero() && d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = d_ref;
	if( d_ref[ DimX ] == 0 )
		tuple = *d_plane.d_tuples->getSel().begin();

	Spin* ref = tuple[ DimX ];
	ACTION_ENABLED_IF( a, !d_show4DPlane && ref->getSystem() && getSpec()->hasNoesy()  );

	ProposeSpinDlg dlg( MW, d_pro, getSpec()->getColor( DimY ), d_cursor[ DimY ],
		getSpec(), "Select Vertical Partner" );
	dlg.setAnchor( DimX, ref );
	if( !dlg.exec() || dlg.getSpin() == 0 )
		return;

	// TODO: wenn target kein SpinSystem hat, dann ins ref einfgen.
	// TODO: wenn target und ref System haben, einen SysLink vorschlagen (Seite?)

	if( ref->findLink( dlg.getSpin() ) == 0 ) // Ref == target zul�sig wegen Diagonaler
	{
		Root::Ref<LinkSpinCmd> cmd = new LinkSpinCmd( d_pro->getSpins(), ref, dlg.getSpin() ); 
		cmd->handle( this );
	}else
		Root::ReportToUser::alert( this, "Propose Vertical Extension", 
			"The selected spins are already linked!" );
}

void FourDScopeAgent::handleProposePeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_show4DPlane && d_spec4D && d_spec4D->hasNoesy() ||
		!d_show4DPlane && d_spec2D && d_spec2D->hasNoesy() );

	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Propose Peak" );
	Dlg::SpinTuple res;
	if( d_show4DPlane || d_spec2D.isNull() )
	{
		Dlg::SpinRanking l1 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_spec4D->getColor( DimX ), d_cursor[ DimX ], d_spec4D );
		Dlg::SpinRanking l2 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_spec4D->getColor( DimY ), d_cursor[ DimY ], d_spec4D );
		Dlg::SpinRanking l3 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_spec4D->getColor( DimZ ), d_cursor[ DimZ ], d_spec4D );
        Dlg::SpinRanking l4 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_spec4D->getColor( DimW ), d_cursor[ DimW ], d_spec4D );
		res = Dlg::selectSpinQuadruple( MW, l1, l2, l3, l4, "Select Matching Spins" );
		if( res.empty() )
			return;

		if( d_spec4D->isNoesy( DimX, DimY ) && res[DimX]->findLink( res[DimY] ) == 0 )
			cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimX], res[DimY] ) ); 

		if( d_spec4D->isNoesy( DimY, DimZ ) && res[DimY]->findLink( res[DimZ] ) == 0 )
			cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimY], res[DimZ] ) ); 

		if( d_spec4D->isNoesy( DimX, DimZ ) && res[DimX]->findLink( res[DimZ] ) == 0 )
			cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimX], res[DimZ] ) ); 

        if( d_spec4D->isNoesy( DimX, DimW ) && res[DimX]->findLink( res[DimW] ) == 0 )
			cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimX], res[DimW] ) );

        if( d_spec4D->isNoesy( DimY, DimW ) && res[DimY]->findLink( res[DimW] ) == 0 )
			cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimY], res[DimW] ) );

        if( d_spec4D->isNoesy( DimZ, DimW ) && res[DimZ]->findLink( res[DimW] ) == 0 )
			cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimZ], res[DimW] ) );
	}else
	{
		Dlg::SpinRanking l1 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_spec2D->getColor( DimX ), d_cursor[ DimX ], d_spec2D );
		Dlg::SpinRanking l2 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_spec2D->getColor( DimY ), d_cursor[ DimY ], d_spec2D );
		res = Dlg::selectSpinPair( MW, 
			l1, l2, "Select Matching Spins" );
		if( res.empty() )
			return;
		if( res[DimX]->findLink( res[DimY] ) == 0 ) // Ref == target zul�sig wegen Diagonaler
			cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimX], res[DimY] ) ); 
	}

	if( cmd->empty() )
	{
		Root::ReportToUser::alert( this, "Propose New Peak", 
			"The selected spins are already linked!" );
		return;
	}
	cmd->handle( this );
}

void FourDScopeAgent::handleSelectSpec4D(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_spec4D == a.getParam( 0 ).getObject() );

	setSpec4D( dynamic_cast<Spectrum*>( a.getParam( 0 ).getObject() ) );
	selectCurSystem();
}

void FourDScopeAgent::handlePickSpin4D(Action & a)
{
	if( !d_src4D->showNulls() )
	{
		handlePickLabel4D( a );
		return;
	}

	const Dimension ref = DimX;
	ACTION_ENABLED_IF( a, !d_spec4D.isNull() && !d_cur.isZero() );

	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick Spins" );
	// Pick generisches Spektrum
	if( d_cur[ ref ]->getSystem() )
	{
		Root::Ref<PickSystemSpinCmd> c2 = new PickSystemSpinCmd( d_pro->getSpins(), 
			d_cur[ ref ]->getSystem(), d_spec4D->getColor( DimZ ), d_cursor[ DimZ ], 0 );
		cmd->add( c2 );
		c2->execute();
        Root::Ref<PickSystemSpinCmd> c3 = new PickSystemSpinCmd( d_pro->getSpins(),
			d_cur[ ref ]->getSystem(), d_spec4D->getColor( DimW ), d_cursor[ DimW ], 0 );
		cmd->add( c3 );
		c3->execute();
	}else
	{
		Root::Ref<PickSpinCmd> c2 = new PickSpinCmd( d_pro->getSpins(), 
			d_spec4D->getColor( DimZ ), d_cursor[ DimZ ], 0 );
		cmd->add( c2 );
		c2->execute();
        Root::Ref<PickSpinCmd> c3 = new PickSpinCmd( d_pro->getSpins(),
			d_spec4D->getColor( DimW ), d_cursor[ DimW ], 0 );
		cmd->add( c3 );
		c3->execute();
	}
	
	cmd->handle( this );
	d_ortho.d_tuples->selectPeak( d_cursor[ DimW ], d_cursor[ DimZ ] );
}

void FourDScopeAgent::handleMoveSpin4D(Action & a)
{
	ACTION_ENABLED_IF( a, d_ortho.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_ortho.d_tuples->getSel().begin();
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spins" );
    // Move generisches Spektrum
	cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ DimX ], d_cursor[ DimW ], 0 ) );
    cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ DimY ], d_cursor[ DimZ ], 0 ) );
	cmd->handle( this );
}

void FourDScopeAgent::handleMoveSpinAlias4D(Action & a)
{
    ACTION_ENABLED_IF( a, d_ortho.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_ortho.d_tuples->getSel().begin();
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spin Aliasses" );
    cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ DimX ], d_cursor[ DimW ], d_spec4D ) );
    cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ DimY ], d_cursor[ DimZ ], d_spec4D ) );
	cmd->handle( this );
}

void FourDScopeAgent::handleDeleteSpins4D(Action & a)
{
	ACTION_ENABLED_IF( a, !d_spec4D.isNull() && !d_ortho.d_tuples->getSel().empty() );

	SpinPointView::Selection sel = d_ortho.d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Spins" );
	std::set<Spin*> test;
	for( p = sel.begin(); p != sel.end(); ++p )
	{
        if( test.count( (*p)[ DimX ] ) == 0 )
		{
			cmd->add( new DeleteSpinCmd( d_pro->getSpins(), (*p)[ DimX ] ) );
			test.insert( (*p)[ DimX ] );
		}
		if( test.count( (*p)[ DimY ] ) == 0 )
		{
			cmd->add( new DeleteSpinCmd( d_pro->getSpins(), (*p)[ DimY ] ) );
			test.insert( (*p)[ DimY ] );
		}
	}
	cmd->handle( this );
}

void FourDScopeAgent::handleLabelSpin4D(Action & a)
{
	ACTION_ENABLED_IF( a, !d_spec4D.isNull() && d_ortho.d_tuples->getSel().size() == 1 );

    SpinPoint tuple = *d_ortho.d_tuples->getSel().begin();

    SpinLabel x = tuple[ DimX ]->getLabel();
	SpinLabel y = tuple[ DimY ]->getLabel();
    SpinLabelSet lx;
    SpinLabelSet ly;
    NmrExperiment* e = d_pro->getRepository()->getTypes()->
		inferExperiment2( d_spec4D->getType(), tuple[ DimX ]->getSystem(), d_spec4D );
	if( e )
		e->getColumn( d_spec4D->mapToType( DimW ), lx );
    e = d_pro->getRepository()->getTypes()->
		inferExperiment2( d_spec4D->getType(), tuple[ DimY ]->getSystem(), d_spec4D );
	if( e )
		e->getColumn( d_spec4D->mapToType( DimZ ), ly );
	if( !Dlg::getLabels( MW, tuple[ DimX ]->getId(), tuple[ DimY ]->getId(), x, y, lx, ly ) )
		return;

    const bool yOk = tuple[ DimY ]->getLabel() == y || // wenn Tag und Offset gleich, kann problemlos gendert werden
            tuple[ DimY ]->getSystem() != 0 && tuple[ DimY ]->getSystem()->isAcceptable( y );
    const bool xOk = tuple[ DimX ]->getLabel() == x ||
            tuple[ DimX ]->getSystem() != 0 && tuple[ DimX ]->getSystem()->isAcceptable( x );
    if( !yOk || !xOk )
    {
        Root::ReportToUser::alert( this, "Label Spin", "One or both labels are not acceptable" );
        return;
    }

    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Label Spins" );
    if( xOk && !tuple[ DimX ]->getLabel().equals( x ) )
		cmd->add( new LabelSpinCmd( d_pro->getSpins(), tuple[ DimX ], x ) );
    if( yOk && !tuple[ DimY ]->getLabel().equals( y ) )
		cmd->add( new LabelSpinCmd( d_pro->getSpins(), tuple[ DimY ], y ) );
	if( !cmd->empty() )
		cmd->handle( this );
	if( x.isNull() || y.isNull() )
		d_src4D->showNulls( true );
}

void FourDScopeAgent::handleSetWidth(Action & a)
{
	ACTION_ENABLED_IF( a, !d_spec4D.isNull() && d_ortho.d_viewer->hasFocus() );

    PpmPoint wxy( d_pro->inferPeakWidth( DimX, d_spec4D ), d_pro->inferPeakWidth( DimY, d_spec4D ),
                  d_pro->inferPeakWidth( DimZ, d_spec4D ),
                  d_pro->inferPeakWidth( DimW, d_spec4D ) );

    if( Dlg::getPpmPoint( MW, wxy, "Set Peak Width", QStringList() << d_spec4D->getDimName(DimX) <<
                          d_spec4D->getDimName(DimY) << d_spec4D->getDimName(DimZ) << d_spec4D->getDimName(DimW ) ) )
    {
        if( wxy[0] < 0.0 || wxy[1] < 0.0 )
        {
            QMessageBox::critical( MW, "Set Peak Width",
                    "Invalid peak width!", "&Cancel" );
            return;
        }
        d_pro->setPeakWidth( DimX, wxy[0], d_spec4D );
        d_pro->setPeakWidth( DimY, wxy[1], d_spec4D );
        d_pro->setPeakWidth( DimZ, wxy[2], d_spec4D );
        d_pro->setPeakWidth( DimW, wxy[3], d_spec4D );

    }
}

void FourDScopeAgent::handleFitWindow4D(Action & a)
{
	ACTION_ENABLED_IF( a, !d_spec4D.isNull() );
	if( d_spec4D.isNull() )
		return;	// QT-BUG
    d_ortho.d_buf->fitToArea();
    d_slices[ DimZ ].d_buf4D->fitToArea();
    d_slices[ DimZ ].d_viewer->redraw();
    d_slices[ DimW ].d_buf4D->fitToArea();
    d_slices[ DimW ].d_viewer->redraw();

//    PpmRange r = d_spec4D->getScale( DimZ ).getRange();
//	r.invert();
//	d_ortho.d_viewer->getViewArea()->setRange( DimY, r );
//	d_slices[ DimZ ].d_viewer->getViewArea()->setRange( DimY, r );
//    r = d_spec4D->getScale( DimW ).getRange();
//	// TODO r.invert();
//	d_ortho.d_viewer->getViewArea()->setRange( DimX, r );
//	d_slices[ DimW ].d_viewer->getViewArea()->setRange( DimX, r );
	d_ortho.d_viewer->redraw();
}

void FourDScopeAgent::handleAutoContour2(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_ortho.d_view->isAuto() );
	
	bool ac = !d_ortho.d_view->isAuto();
	updateContour4D( ac );
	if( d_show4DPlane )
		updateContour( 0, true );
}

void FourDScopeAgent::handleContourParams2(Action & a)
{
	ACTION_ENABLED_IF( a, d_spec4D );

	Dlg::ContourParams p;
	p.d_factor = d_ortho.d_view->getFactor();
	p.d_threshold =	d_spec4D->getThreshold();
	p.d_option = d_ortho.d_view->getOption();
	if( Dlg::setParams( MW, p ) )
	{
		d_spec4D->setThreshold( p.d_threshold );
		d_ortho.d_view->setOption( p.d_option );
		d_ortho.d_view->setFactor( p.d_factor );
		updateContour4D( false );
		if( d_show4DPlane )
		{
			showIntens( false );
			updateContour( 0, true );
		}
	}
}

void FourDScopeAgent::handleShowWithOff(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src4D->showOffs() );
	
	d_src4D->showOffs( !d_src4D->showOffs() );
}

void FourDScopeAgent::setShow4dPlane(bool on)
{
	if( on && d_spec4D )
	{
        if( d_show4DPlane != on )
        {
            d_contourFactor = d_plane.d_ol[0].d_view->getFactor();
            d_contourOption = d_plane.d_ol[0].d_view->getOption();
            d_gain = d_plane.d_ol[0].d_view->getGain();
            d_autoContour = d_plane.d_ol[0].d_view->isAuto();
        }

        d_plane.d_ol[0].d_spec = new SpecProjector( d_spec4D, DimX, DimY );
		d_plane.d_ol[0].d_spec->setOrigin( d_cursor );
		d_plane.d_ol[0].d_buf->setSpectrum( d_plane.d_ol[0].d_spec );
        d_plane.d_tuples->setModel( d_plane.d_mdl4D );
        d_plane.d_pp->setOrigThick( DimZ, d_cursor[ DimZ ], d_spec4D->getScale( DimZ ).getDelta() );
        d_plane.d_pp->setOrigThick( DimW, d_cursor[ DimW ], d_spec4D->getScale( DimW ).getDelta() );
        d_plane.d_pp->setDimGhostWidth( DimZ, DimW, d_plane.d_mdl4D->getGhostWidth()[0],
                                d_plane.d_mdl4D->getGhostWidth()[1] );
		d_plane.d_peaks->setSpec( d_spec4D );
    }else if( d_spec2D )
	{
		if( d_autoContour )
		{
			d_plane.d_ol[0].d_view->createLevelsAuto( d_contourFactor,
				d_contourOption, d_gain );
		}else
			d_plane.d_ol[0].d_view->createLevelsMin( d_contourFactor,
				d_spec2D->getThreshold(), d_contourOption );
        d_plane.d_ol[0].d_spec = new SpecProjector( d_spec2D, DimX, DimY );
		d_plane.d_ol[0].d_buf->setSpectrum( d_plane.d_ol[0].d_spec );
        d_plane.d_tuples->setModel( d_mdl2D );
        d_plane.d_pp->setOrigThick( DimZ, d_cursor[ DimZ ], 0 );
        d_plane.d_pp->setOrigThick( DimW, d_cursor[ DimW ], 0 );
        d_plane.d_pp->setDimGhostWidth( DimZ, DimW, 0, 0 );
		d_plane.d_peaks->setSpec( d_spec2D );
	}
    d_show4DPlane = on;
	d_plane.d_vRulerMdl->setSpec( d_plane.d_ol[0].d_spec );
	d_plane.d_hRulerMdl->setSpec( d_plane.d_ol[0].d_spec );
	updateContour( 0, true );
	updatePlaneLabel();
    d_plane.d_viewer->redraw();
	SpecChanged msg( true );
    traverseUp( msg );
}

void FourDScopeAgent::handleShowPathSim(Action & a)
{
    ACTION_ENABLED_IF( a, d_ortho.d_viewer->hasFocus() && !d_spec4D.isNull() ||
                d_plane.d_viewer->hasFocus() );

    PathSimDlg dlg( (d_ortho.d_viewer->hasFocus())?d_spec4D.deref():getSpec(), MW );
    if( d_cur[DimX] && d_cur[DimX]->getSystem() && d_cur[DimX]->getSystem()->getAssig() )
        dlg.setResiType( d_cur[DimX]->getSystem()->getAssig()->getType() );
    dlg.exec();
}

void FourDScopeAgent::handleShow4dPlane(Action & a)
{
    // Einschalten nur wenn 4D vorhanden; ausschalten nur wenn 2D vorhanden
	ACTION_CHECKED_IF( a, !d_show4DPlane && d_spec4D || d_show4DPlane && d_spec2D, d_show4DPlane );
    setShow4dPlane( !d_show4DPlane );
}

void FourDScopeAgent::handleAutoHide(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_autoHide );

	d_autoHide = !d_autoHide;
	if( d_spec4D )
	{
		sync4dXySliceToCur( DimX, !d_autoHide );
		sync4dXySliceToCur( DimY, !d_autoHide );
	}
	d_slices[ DimX ].d_viewer->redraw();
	d_slices[ DimY ].d_viewer->redraw();
}

void FourDScopeAgent::handleStripCalibrate(Action & a)
{
    ACTION_ENABLED_IF( a, d_ortho.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_ortho.d_tuples->getSel().begin();

	PpmPoint p( 0, 0, 0, 0 );
	p[ DimZ ] = tuple[ DimY ]->getShift( d_spec4D ) - d_cursor[ DimZ ];
    p[ DimW ] = tuple[ DimX ]->getShift( d_spec4D ) - d_cursor[ DimW ];

	Viewport::pushHourglass();
	Root::Ref<SpecCalibrateCmd> cmd = new SpecCalibrateCmd( d_spec4D, p );
	cmd->handle( this );
	Viewport::popCursor();
}

void FourDScopeAgent::handleProposeSpin(Action & a)
{
	ACTION_ENABLED_IF( a, !d_spec4D.isNull() && !d_cur.isZero() && d_spec4D->hasNoesy() );

    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Propose Spins" );
	Dlg::SpinTuple res;

    Dlg::SpinRanking l3 = d_pro->getMatcher()->findMatchingSpins(
                d_pro->getSpins(), d_spec4D->getColor( DimZ ), d_cursor[ DimZ ], d_spec4D );
    Dlg::SpinRanking l4 = d_pro->getMatcher()->findMatchingSpins(
                d_pro->getSpins(), d_spec4D->getColor( DimW ), d_cursor[ DimW ], d_spec4D );
    res = Dlg::selectSpinPair( MW, l4, l3, "Select Matching Spins" );
    if( res.empty() )
        return;
    const Dimension DW = DimX;
    const Dimension DZ = DimY;

    if( d_spec4D->isNoesy( DimX, DimZ ) && d_cur[ DimX ]->findLink( res[DZ] ) == 0 )
        cmd->add( new LinkSpinCmd( d_pro->getSpins(), d_cur[ DimX ], res[DZ] ) );

    if( d_spec4D->isNoesy( DimY, DimZ ) && d_cur[ DimY ]->findLink( res[DZ] ) == 0 )
        cmd->add( new LinkSpinCmd( d_pro->getSpins(), d_cur[ DimY ], res[DZ] ) );

    if( d_spec4D->isNoesy( DimX, DimW ) && d_cur[ DimX ]->findLink( res[DW] ) == 0 )
        cmd->add( new LinkSpinCmd( d_pro->getSpins(), d_cur[ DimX ], res[DW] ) );

    if( d_spec4D->isNoesy( DimY, DimW ) && d_cur[ DimY ]->findLink( res[DW] ) == 0 )
        cmd->add( new LinkSpinCmd( d_pro->getSpins(), d_cur[ DimY ], res[DW] ) );

    if( d_spec4D->isNoesy( DimZ, DimW ) && res[DZ]->findLink( res[DW] ) == 0 )
        cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DZ], res[DW] ) );

    if( cmd->empty() )
	{
		Root::ReportToUser::alert( this, "Propose Spins",
			"The selected spins are already linked!" );
		return;
	}
	cmd->handle( this );
}

void FourDScopeAgent::handleEditAttsSpinH(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimX ] );
}

void FourDScopeAgent::handleEditAttsSpinV(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimY ] );
}

void FourDScopeAgent::handleEditAttsLink(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
	ACTION_ENABLED_IF( a, l );

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keyLink ), l );
}

void FourDScopeAgent::handleEditAttsSysH(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinSystem* s = tuple[ DimX ]->getSystem();
	ACTION_ENABLED_IF( a, s );

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ), s );
}

void FourDScopeAgent::handleEditAttsSysV(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinSystem* s = tuple[ DimY ]->getSystem();
	ACTION_ENABLED_IF( a, s );

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ), s );
}

void FourDScopeAgent::handleEditAttsSpinX4D(Action & a)
{
    ACTION_ENABLED_IF( a, d_ortho.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_ortho.d_tuples->getSel().begin();

	DynValueEditor::edit( MW,
		d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimX ] );
}

void FourDScopeAgent::handleEditAttsSpinY4D(Action & a)
{
    ACTION_ENABLED_IF( a, d_ortho.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_ortho.d_tuples->getSel().begin();

	DynValueEditor::edit( MW,
		d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimY ] );
}

void FourDScopeAgent::handleCursorSync(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_cursorSync );
	
	d_cursorSync = !d_cursorSync;
	if( d_cursorSync )
		GlobalCursor::addObserver( this );	// TODO: preset Cursor
	else if( !d_rangeSync )
		GlobalCursor::removeObserver( this );
}

void FourDScopeAgent::handleGotoSystem(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	QString id;
	if( a.getParam( 0 ).isNull() )
	{
		bool ok	= FALSE;
		id	= QInputDialog::getText( "Goto System", 
			"Please	enter a spin system id:", QLineEdit::Normal, "", &ok, MW );
		if( !ok )
			return;
	}else
		id = a.getParam( 0 ).getCStr();

	SpinSystem* sys = d_pro->getSpins()->findSystem( id );
	if( sys == 0 )
	{
		Lexi::ShowStatusMessage msg( "Goto System: unknown system" );
		traverseUp( msg );
		return;
	}
	gotoTuple( sys, 0, 0, true );
}

void FourDScopeAgent::handleNextSpec4D(Action & a)
{
	ACTION_ENABLED_IF( a, d_sort4D.size() > 1 );
	stepSpec4D( true );
}

void FourDScopeAgent::handlePrevSpec4D(Action & a)
{
	ACTION_ENABLED_IF( a, d_sort4D.size() > 1 );
	stepSpec4D( false );
}

void FourDScopeAgent::handleNextSpec2D(Action & a)
{
	ACTION_ENABLED_IF( a, d_sort2D.size() > 1 ); 
	stepSpec2D( true );
}

void FourDScopeAgent::handlePrevSpec2D(Action & a)
{
	ACTION_ENABLED_IF( a, d_sort2D.size() > 1 ); 
	stepSpec2D( false );
}

void FourDScopeAgent::handleShowWithOff2(Action & a)
{
	ACTION_CHECKED_IF( a, !d_show4DPlane, d_src2D->showOffs() );
	
	d_src2D->showOffs( !d_src2D->showOffs() );
	d_plane.d_viewer->redraw();
}

void FourDScopeAgent::handleShowLinks(Action & a)
{
	ACTION_CHECKED_IF( a, !d_show4DPlane, d_src2D->showLinks() );
	
	d_src2D->showLinks( !d_src2D->showLinks() );
	d_plane.d_viewer->redraw();
}

void FourDScopeAgent::handleShowLinks2(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src4D->showLinks() );
	
	d_src4D->showLinks( !d_src4D->showLinks() );
	d_plane.d_viewer->redraw();
}

void FourDScopeAgent::handleDeleteLinks(Action & a)
{
	ACTION_ENABLED_IF( a, !d_plane.d_tuples->getSel().empty() );

	SpinPointView::Selection sel = d_plane.d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
	SpinBase* base = d_pro->getSpins();
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Links" );
	std::set<SpinLink*> test;
	SpinLink* l;
	for( p = sel.begin(); p != sel.end(); ++p )
	{
		l = (*p)[ DimX ]->findLink( (*p)[ DimY ] );
		if( l && test.count( l ) == 0 )
		{
			cmd->add( new UnlinkSpinCmd( base, (*p)[ DimX ], (*p)[ DimY ] ) );
			test.insert( l );
		}
	}
	cmd->handle( this );
}

void FourDScopeAgent::handleLabelVerti(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	SpinLabel l;
	if( !SpinLabel::parse( a.getParam( 0 ).getCStr(), l ) )
	{
		Root::ReportToUser::alert( this, "Label Vertical Spin", "Invalid spin label syntax!" );
		return;
	}
	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_pro->getSpins(), tuple[ DimY ], l );
	cmd->handle( this );
	if( l.isNull())
		d_src2D->showNulls( true );
}

void FourDScopeAgent::handleLabelHori(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	SpinLabel l;
	if( !SpinLabel::parse( a.getParam( 0 ).getCStr(), l ) )
	{
		Root::ReportToUser::alert( this, "Label Horizontal Spin", "Invalid spin label syntax!" );
		return;
	}
	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_pro->getSpins(), tuple[ DimX ], l );
	cmd->handle( this );
	if( l.isNull() )
		d_src2D->showNulls( true );
}

void FourDScopeAgent::handleSetCandidates(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	ACTION_ENABLED_IF( a, tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

	SpinSystem* sys = tuple[ DimX ]->getSystem();

	CandidateDlg dlg( MW, d_pro->getRepository() );
	dlg.setTitle( sys );
	if( dlg.exec() )
		d_pro->getSpins()->setCands( sys, dlg.d_cands );
}

void FourDScopeAgent::handleShowInfered(Action & a)
{
	ACTION_CHECKED_IF( a, !d_show4DPlane, d_src2D->showInferred() );
	
	d_src2D->showInferred( !d_src2D->showInferred() );
	d_plane.d_viewer->redraw();
}

void FourDScopeAgent::handleShowInfered2(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src4D->showInferred() );
	
	d_src4D->showInferred( !d_src4D->showInferred() );
	d_plane.d_viewer->redraw();
}

void FourDScopeAgent::handleShowUnlabeled(Action & a)
{
	ACTION_CHECKED_IF( a, !d_show4DPlane, d_src2D->showNulls() );
	
	d_src2D->showNulls( !d_src2D->showNulls() );
	d_plane.d_viewer->redraw();
}

void FourDScopeAgent::handleShowUnlabeled2(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src4D->showNulls() );
	
	d_src4D->showNulls( !d_src4D->showNulls() );
	d_plane.d_viewer->redraw();
}

void FourDScopeAgent::handleShowUnknown(Action & a)
{
	ACTION_CHECKED_IF( a, !d_show4DPlane, d_src2D->showUnknown() );
	
	d_src2D->showUnknown( !d_src2D->showUnknown() );
	d_plane.d_viewer->redraw();
}

void FourDScopeAgent::handleShowUnknown2(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src4D->showUnknown() );
	
	d_src4D->showUnknown( !d_src4D->showUnknown() );
	d_plane.d_viewer->redraw();
}

void FourDScopeAgent::handleCreateLinks(Action & a)
{
	ACTION_CHECKED_IF( a, true, false );
}

void FourDScopeAgent::handleForceCross(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src2D->doPathsim() );
	
	d_src2D->doPathsim( !d_src2D->doPathsim() );
	d_src4D->doPathsim( d_src2D->doPathsim() );
	d_plane.d_viewer->redraw();
}

void FourDScopeAgent::handleDeleteLinks4D(Action & a)
{
	ACTION_ENABLED_IF( a, !d_spec4D.isNull() && !d_ortho.d_tuples->getSel().empty() );

	SpinPointView::Selection sel = d_ortho.d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Links" );
	SpinLink* l;
	std::set<SpinLink*> test;
	for( p = sel.begin(); p != sel.end(); ++p )
	{
		l = (*p)[ DimX ]->findLink( (*p)[ DimY ] );
		if( l && test.count( l ) == 0 )
		{
			cmd->add( new UnlinkSpinCmd( d_pro->getSpins(), (*p)[ DimX ], (*p)[ DimY ] ) );
			test.insert( l );
		}
	}
	cmd->handle( this );
}

void FourDScopeAgent::handleViewLabels4D(Action & a)
{
	if( a.getParamCount() == 0 )
		return;

	SpinPointView::Label q = (SpinPointView::Label) a.getParam( 0 ).getShort();
	if( q < SpinPointView::None || q >= SpinPointView::End )
		return;

	ACTION_CHECKED_IF( a, true,
		d_ortho.d_tuples->getLabel() == q );
	
    d_ortho.d_tuples->setLabel( q, DimY );
    d_ortho.d_viewer->redraw();
}

void FourDScopeAgent::handleAutoGain(Action & a)
{
	ACTION_ENABLED_IF( a, !a.getParam( 0 ).isNull() && !d_show4DPlane );

	float g = a.getParam( 0 ).getFloat();
	if( g <= 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Auto Gain", "Invalid Gain Value" );
		return;
	}
	int l = d_aol;
	if( a.getParam( 1 ).isNull() )
		l = selectLayer();
	else if( ::strcmp( a.getParam( 1 ).getCStr(), "*" ) == 0 )
		l = -1;
	else
		l = a.getParam( 1 ).getLong();
	if( l == -2 )
		return;
	else if( l == -1 ) // All
	{
		for( l = 0; l < d_plane.d_ol.size(); l++ )
		{
			d_plane.d_ol[l].d_view->setVisi( true );
			d_plane.d_ol[l].d_view->setGain( g );
			d_plane.d_ol[l].d_view->createLevelsAuto();
		}
	}else if( l >= 0 && l < d_plane.d_ol.size() )
	{
		d_plane.d_ol[l].d_view->setVisi( true );
		d_plane.d_ol[l].d_view->setGain( g );
		d_plane.d_ol[l].d_view->createLevelsAuto();
	}
	d_plane.d_viewer->damageMe();
}

void FourDScopeAgent::handleAutoGain4D(Action & a)
{
	ACTION_ENABLED_IF( a, !a.getParam( 0 ).isNull() );
	float g = a.getParam( 0 ).getFloat();
	if( g <= 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Auto Gain", "Invalid Gain Value" );
		return;
	}
	d_ortho.d_view->setGain( g );
	updateContour4D( true );
}

void FourDScopeAgent::handleShowGhosts(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_plane.d_mdl4D->showGhosts() );

	d_plane.d_mdl4D->showGhosts( !d_plane.d_mdl4D->showGhosts() );
	d_ortho.d_mdl->showGhosts( d_plane.d_mdl4D->showGhosts() );
}

void FourDScopeAgent::handleAutoHold(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_autoHold );
	d_autoHold = !d_autoHold;
}

#ifdef _oldVersion
void FourDScopeAgent::handlePickLabel4D(Action & a)
{
    SpinSystem* sys = 0;
    if( !d_cur.isZero() && d_cur[ DimX ] )
        sys = d_cur[ DimX ]->getSystem();
    ACTION_ENABLED_IF( a, !d_spec4D.isNull() && sys );

    SpinLabelSet lx = d_spec4D->getType()->getLabels( d_spec4D->mapToType( DimW ) );
	SpinLabelSet ly = d_spec4D->getType()->getLabels( d_spec4D->mapToType( DimZ ) );
    NmrExperiment* e = d_pro->getRepository()->getTypes()->inferExperiment2( d_spec4D->getType(), sys, 0, d_spec4D );
	if( e )
	{
		PathTable::Path filter( 4 );
		filter[ d_spec4D->mapToType( DimX ) ] = d_cur[ DimX ]->getLabel();
		filter[ d_spec4D->mapToType( DimY ) ] = d_cur[ DimY ]->getLabel();
        // wir wollen nur Spalten, welche die Labels der in der Plane selektierten Spins enthalten
		e->getColumn( filter, d_spec4D->mapToType( DimZ ), ly );
        e->getColumn( filter, d_spec4D->mapToType( DimW ), lx );
	}
    // lx und ly enthalten hier alle grundstzlich mglichen Labels, unabhngig davon, ob diese bereits gepickt wurden
    if( sys )
    {
        // wenn ein System ausgewhlt ist, behalte nur die noch freien Labels
        ly = sys->getAcceptables( ly );
        lx = sys->getAcceptables( lx );
    }
    SpinLabel x;
	SpinLabel y;
    if( !Dlg::getLabels( MW, 0, 0, x, y, lx, ly ) )
		return;
    if( !sys->isAcceptable( y ) || !sys->isAcceptable( x ) )
	{
        // TODO: warum dieser Test?
		Root::ReportToUser::alert( this, "Pick Label", "Label is not acceptable" );
		return;
	}

    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick Label" );
	Root::Ref<PickSystemSpinLabelCmd> c1 = new PickSystemSpinLabelCmd( d_pro->getSpins(),
        sys, d_spec4D->getColor( DimZ ), d_cursor[ DimZ ], y, 0 );
    cmd->add( c1 );
    Root::Ref<PickSystemSpinLabelCmd> c2 = new PickSystemSpinLabelCmd( d_pro->getSpins(),
        sys, d_spec4D->getColor( DimW ), d_cursor[ DimW ], x, 0 );
    cmd->add( c2 );
    cmd->handle( this );

    if( x.isNull() || y.isNull() )
		d_src4D->showNulls( true );

	d_ortho.d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimY ] );
}
#else
void FourDScopeAgent::handlePickLabel4D(Action & a)
{
    SpinSystem* sys = 0;
    if( !d_cur.isZero() )
        sys = d_cur[ DimX ]->getSystem();
    ACTION_ENABLED_IF( a, !d_spec4D.isNull() && sys );

    SpinLabelPoints pairs;
    SpinLabelPoint pair;
	NmrExperiment* e = d_pro->getRepository()->getTypes()->inferExperiment2( d_spec4D->getType(), sys, d_spec4D );
    if( e )
    {
        PathTable::Path filter;
        filter[ d_spec4D->mapToType( DimX ) ] = d_cur[ DimX ]->getLabel();
        filter[ d_spec4D->mapToType( DimY ) ] = d_cur[ DimY ]->getLabel();
        // wir wollen nur Spalten, welche die Labels der in der Plane selektierten Spins enthalten
        pairs = e->getTuples( filter, d_spec4D->mapToType( DimW ), d_spec4D->mapToType( DimZ ) );
    }
    // pairs enthalten hier alle grundstzlich mglichen Labels, unabhngig davon, ob diese bereits gepickt wurden
    // wenn ein System ausgewhlt ist, behalte nur die noch freien Labels
    pairs = sys->getAcceptables( pairs, 2 );
    if( !Dlg::getLabelPoint( MW, pairs, pair, 2, "Pick Labels" ) )
        return;

    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick Labels" );
    if( sys->isAcceptable( pair[1] ) )
        cmd->add( new PickSystemSpinLabelCmd( d_pro->getSpins(),
                                              sys, d_spec4D->getColor( DimZ ), d_cursor[ DimZ ], pair[1], 0 ) );
    if( sys->isAcceptable( pair[0] ) )
        cmd->add( new PickSystemSpinLabelCmd( d_pro->getSpins(),
                                              sys, d_spec4D->getColor( DimW ), d_cursor[ DimW ], pair[0], 0 ) );
    if( !cmd->empty() )
        cmd->handle( this );

    d_ortho.d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimY ] );
}
#endif

void FourDScopeAgent::handleGhostLabels(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_plane.d_tuples->ghostLabel() );

	d_plane.d_tuples->ghostLabel( !d_plane.d_tuples->ghostLabel() );
	d_ortho.d_tuples->ghostLabel( d_plane.d_tuples->ghostLabel() );
}

void FourDScopeAgent::handleHidePeak4D(Action & a)
{
    if( d_ortho.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_ortho.d_tuples->getSel().begin();
	SpinLink* link = tuple[ DimX ]->findLink( tuple[ DimY ] );
	ACTION_ENABLED_IF( a, link );
	Root::Ref<HideSpinLinkCmd> cmd = new HideSpinLinkCmd( d_pro->getSpins(), link, d_spec4D );
	cmd->handle( this );
	// TODO: Plural
}

void FourDScopeAgent::handleGotoPeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_spec4D );

	QString id;
	if( a.getParam( 0 ).isNull() )
	{
		bool ok	= FALSE;
		id	= QInputDialog::getText( "Goto Peak", 
			"Please	enter a spin system id:",QLineEdit::Normal, "", &ok, MW );
		if( !ok )
			return;
	}else
		id = a.getParam( 0 ).getCStr();

	SpinSystem* sys = d_pro->getSpins()->findSystem( id );
	if( sys == 0 )
	{
		Lexi::ShowStatusMessage msg( "Goto Peak: unknown system" );
		traverseUp( msg );
		return;
	}
	gotoTuple( sys, 0, 0, false );
}

void FourDScopeAgent::handleRangeSync(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_rangeSync );
	
	d_rangeSync = !d_rangeSync;
	if( d_rangeSync )
		GlobalCursor::addObserver( this );	// TODO: preset Cursor
	else if( !d_cursorSync )
		GlobalCursor::removeObserver( this );
}

void FourDScopeAgent::handleEditAttsSys4D(Action & a)
{
	ACTION_ENABLED_IF( a, !d_spec4D.isNull() && !d_cur.isZero() && d_cur[DimX]->getSystem() );

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ), 
		d_cur[DimX]->getSystem() );
}

void FourDScopeAgent::handleEditAttsLink4D(Action & a)
{
    if( d_ortho.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_ortho.d_tuples->getSel().begin();
	SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
	ACTION_ENABLED_IF( a, l );

	DynValueEditor::edit( MW,
		d_pro->getRepository()->findObjectDef( Repository::keyLink ), l );
}

void FourDScopeAgent::handleOverlayCount(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	Root::Index c;
	if( a.getParam( 0 ).isNull() )
	{
		bool ok	= FALSE;
		c = QInputDialog::getInteger( "Set Overlay Count", 
			"Please	enter a positive number:", 
			d_plane.d_ol.size(), 1, 9, 1, &ok, MW );
		if( !ok )
			return;
	}else
		c = a.getParam( 0 ).getLong();
	if( c < 1 )
	{
		QMessageBox::critical( MW, "Set Overlay Count",
				"Invalid Count!", "&Cancel" );
		return;
	}
	initOverlay( c );
}

void FourDScopeAgent::handleActiveOverlay(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	Root::Index c;
	if( a.getParam( 0 ).isNull() )
	{
		Dlg::StringList l( d_plane.d_ol.size() );
		QString str;
		for( int i = 0; i < d_plane.d_ol.size(); i++ )
		{
			if( d_plane.d_ol[ i ].d_spec )
				str.sprintf( "&%d %s", i, d_plane.d_ol[ i ].d_spec->getName() );
			else
				str.sprintf( "&%d <empty>", i );
			l[ i ] = str.toLatin1();
		}
		c = Dlg::getOption( MW, l, 
			"Select Active Overlay", d_aol );
		if( c == -1 )
			return;
	}else
		c = a.getParam( 0 ).getLong();
	if( c < 0 || c >= d_plane.d_ol.size() )
	{
		QMessageBox::critical( MW, "Set Active Overlay",
				"Invalid Overlay Number!", "&Cancel" );
		return;
	}
	setActiveOverlay( c );
}

void FourDScopeAgent::handleSetPosColor(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	
	QColor clr = QColorDialog::getColor( d_plane.d_ol[d_aol].d_view->getPosColor(), 
		MW );
	if( clr.isValid() )
	{
		d_plane.d_ol[d_aol].d_view->setPosColor( ( clr ) );
		d_plane.d_viewer->redraw();
	}
}

void FourDScopeAgent::handleSetNegColor(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	QColor clr = QColorDialog::getColor( d_plane.d_ol[d_aol].d_view->getNegColor(), 
		MW );
	if( clr.isValid() )
	{
		d_plane.d_ol[d_aol].d_view->setNegColor( ( clr ) );
		d_plane.d_viewer->redraw();
	}
}

void FourDScopeAgent::handleOverlaySpec(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	Spectrum* spec = d_plane.d_ol[d_aol].d_spec;
	Root::Index c = 0;
	if( a.getParam( 0 ).isNull() )
	{
		Dlg::StringSet s;
		Project::SpectrumMap::const_iterator i;
		s.insert( "" ); // empty
		for( i = d_pro->getSpectra().begin(); i != d_pro->getSpectra().end(); ++i )
			if( (*i).second->getDimCount() == 2 )
				s.insert( (*i).second->getName() );
		if( !Dlg::selectStrings( MW, 
			"Select Overlay Spectrum", s, false ) || s.empty() )
			return;
		spec = d_pro->findSpectrum( (*s.begin()).data() );
	}else
	{
		c = a.getParam( 0 ).getLong();

		spec = d_pro->getSpec( c );
		if( spec == 0 && c != 0 )
		{
			QMessageBox::critical( MW, "Set Overlay Spectrum",
					"Invalid spectrum ID!", "&Cancel" );
			return;
		}
		if( spec && spec->getDimCount() != 2 )
		{
			QMessageBox::critical( MW, "Set Overlay Spectrum",
					"Invalid number of dimensions!", "&Cancel" );
			return;
		}
	}
	if( d_aol == 0 && spec == 0 )
	{
		QMessageBox::critical( MW, "Set Overlay Spectrum",
				"Cannot remove spectrum of layer 0!", "&Cancel" );
		return;
	}
	if( d_aol == 0 )
		setSpec2D( spec );
	else if( spec )
	{
		d_plane.d_ol[d_aol].d_spec = new SpecProjector( spec, DimX, DimY );
		d_plane.d_ol[d_aol].d_buf->setSpectrum( d_plane.d_ol[d_aol].d_spec );
		d_plane.d_viewer->redraw();
	}else
	{
		d_plane.d_ol[d_aol].d_spec = 0;
		d_plane.d_ol[d_aol].d_buf->setSpectrum( 0 );
		d_plane.d_viewer->redraw();
	}
	updatePlaneLabel();
}

void FourDScopeAgent::handleCntFactor(Action & a)
{
	ACTION_ENABLED_IF( a, !d_show4DPlane && !a.getParam( 0 ).isNull() );

	float g = a.getParam( 0 ).getFloat();
	if( g <= 1.0 || g > 10.0 )
	{
		Root::ReportToUser::alert( this, "Set Contour Factor", "Invalid Factor Value" );
		return;
	}
	int l = d_aol;
	if( a.getParam( 1 ).isNull() )
		l = selectLayer();
	else if( ::strcmp( a.getParam( 1 ).getCStr(), "*" ) == 0 )
		l = -1;
	else
		l = a.getParam( 1 ).getLong();
	if( l == -2 )
		return;
	else if( l == -1 ) // All
	{
		for( l = 0; l < d_plane.d_ol.size(); l++ )
		{
			d_plane.d_ol[l].d_view->setFactor( g );
			d_plane.d_ol[l].d_view->setVisi( true );
			updateContour( l, false );
		}
		d_plane.d_viewer->damageMe();
	}else if( l >= 0 && l < d_plane.d_ol.size() )
	{
		d_plane.d_ol[l].d_view->setFactor( g );
		d_plane.d_ol[l].d_view->setVisi( true );
		updateContour( l, true );
	}
}

void FourDScopeAgent::handleCntThreshold(Action & a)
{
	ACTION_ENABLED_IF( a, !d_show4DPlane && !a.getParam( 0 ).isNull() );

	float g = a.getParam( 0 ).getFloat();
	if( g < 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Spectrum Threshold", "Invalid Threshold Value" );
		return;
	}
	int l = d_aol;
	if( a.getParam( 1 ).isNull() )
		l = selectLayer();
	else if( ::strcmp( a.getParam( 1 ).getCStr(), "*" ) == 0 )
		l = -1;
	else
		l = a.getParam( 1 ).getLong();
	if( l == -2 )
		return;
	else if( l == -1 ) // All
	{
		for( l = 0; l < d_plane.d_ol.size(); l++ )
		{
			if( d_plane.d_ol[l].d_spec )
				d_plane.d_ol[l].d_spec->setThreshold( g );
			d_plane.d_ol[l].d_view->setVisi( true );
			d_plane.d_ol[l].d_view->createLevelsMin( g );
		}
	}else if( l >= 0 && l < d_plane.d_ol.size() && d_plane.d_ol[l].d_spec )
	{
		d_plane.d_ol[l].d_spec->setThreshold( g );
		d_plane.d_ol[l].d_view->setVisi( true );
		d_plane.d_ol[l].d_view->createLevelsMin( g );
	}
	d_plane.d_viewer->damageMe();
}

void FourDScopeAgent::handleCntOption(Action & a)
{
	ACTION_ENABLED_IF( a, !d_show4DPlane && !a.getParam( 0 ).isNull() );

	ContourView::Option o = ContourView::Both;
	if( strcmp( a.getParam(0).getCStr(), "+" ) == 0 )
		o = ContourView::Positive;
	else if( strcmp( a.getParam(0).getCStr(), "-" ) == 0 )
		o = ContourView::Negative;
	
	int l = d_aol;
	if( a.getParam( 1 ).isNull() )
		l = selectLayer();
	else if( ::strcmp( a.getParam( 1 ).getCStr(), "*" ) == 0 )
		l = -1;
	else
		l = a.getParam( 1 ).getLong();
	if( l == -2 )
		return;
	else if( l == -1 ) // All
	{
		for( l = 0; l < d_plane.d_ol.size(); l++ )
		{
			d_plane.d_ol[l].d_view->setOption( o );
			d_plane.d_ol[l].d_view->setVisi( true );
			updateContour( l, false );
		}
		d_plane.d_viewer->damageMe();
	}else if( l >= 0 && l < d_plane.d_ol.size() )
	{
		d_plane.d_ol[l].d_view->setOption( o );
		d_plane.d_ol[l].d_view->setVisi( true );
		updateContour( l, true );
	}
}

void FourDScopeAgent::handleAddLayer(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	initOverlay( d_plane.d_ol.size() + 1 );
	setActiveOverlay( d_plane.d_ol.size() - 1 );
	handleOverlaySpec( a );
}

void FourDScopeAgent::handleComposeLayers(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	ColorMap cm( 2 );
	Root::Ref<PeakList> pl = new PeakList( cm );
	PeakList::SpecList l;
	for( int i = 0; i < d_plane.d_ol.size(); i++ )
		if( d_plane.d_ol[i].d_spec )
			l.push_back( d_plane.d_ol[i].d_spec->getId() );
	pl->setSpecs( l );
	SpecBatchList dlg( MW, pl, d_pro );
	dlg.setCaption( "Compose Layers" );
	if( dlg.doit() && !pl->getSpecs().empty() )
	{
		const PeakList::SpecList& s = pl->getSpecs();
		initOverlay( s.size() );
		Spectrum* spec;
		for( int i = 0; i < s.size(); i++ )
		{
			spec = d_pro->getSpec( s[ i ] );
			assert( spec );
			d_plane.d_ol[i].d_spec = new SpecProjector( spec, DimX, DimY );
			d_plane.d_ol[i].d_buf->setSpectrum( d_plane.d_ol[i].d_spec );
		}
		d_plane.d_viewer->redraw();
		updatePlaneLabel();
	}
}

void FourDScopeAgent::handleUseLinkColors(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_plane.d_tuples->getColors() );
	if( d_plane.d_tuples->getColors() )
		d_plane.d_tuples->setColors( 0 );
	else
		d_plane.d_tuples->setColors( d_pro->getRepository()->getColors() );
}

void FourDScopeAgent::handleUseLinkColors4D(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_ortho.d_tuples->getColors() );

	if( d_ortho.d_tuples->getColors() )
	{	
		d_ortho.d_tuples->setColors( 0 );
	}else
	{
		d_ortho.d_tuples->setColors( d_pro->getRepository()->getColors() );
	}
}

void FourDScopeAgent::handleSetLinkParams(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
	ACTION_ENABLED_IF( a, l );
    const SpinLink::Alias& al = l->getAlias( getSpec() );
	Dlg::LinkParams2 par;
	par.d_rating = al.d_rating;
	par.d_code = al.d_code;
	par.d_visible = al.d_visible;
	if( Dlg::getLinkParams2( MW, par ) )
        d_pro->getSpins()->setAlias( l, getSpec(),
		par.d_rating, par.d_code, par.d_visible );
}

void FourDScopeAgent::handleSetLinkParams4D(Action & a)
{
    if( d_spec4D.isNull() || d_ortho.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_ortho.d_tuples->getSel().begin();
	SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
	ACTION_ENABLED_IF( a, l );
    const SpinLink::Alias& al = l->getAlias( d_spec4D );
	Dlg::LinkParams2 par;
	par.d_rating = al.d_rating;
	par.d_code = al.d_code;
	par.d_visible = al.d_visible;
	if( Dlg::getLinkParams2( MW, par ) )
        d_pro->getSpins()->setAlias( l, d_spec4D,
		par.d_rating, par.d_code, par.d_visible );
}

void FourDScopeAgent::handleGotoPoint(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	PpmPoint orig = d_cursor;
	GotoDlg dlg( MW );
	Spectrum* spec = d_spec2D;
	if( d_spec4D )
		spec = d_spec4D;
	PpmCube cube;
	spec->getCube( cube );
	Dimension d;
	for( d = 0; d <	spec->getDimCount(); d++	)
		dlg.addDimension( cube[	d ], spec->getColor( d ), 
			orig[ d ] );
	if( dlg.exec() )
	{
		for( d = 0; d <	spec->getDimCount(); d++ )
		{
			orig[ d ] = dlg.getValue( d );
		}
		setCursor( orig );
	}
}

void FourDScopeAgent::handleNewPeakList(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	if( !askToClosePeaklist() )
		return;

	Spectrum* spec = d_spec2D;
	if( d_spec4D && d_show4DPlane )
		spec = d_spec4D;
	setPeakList( new PeakList( spec ) );
}

void FourDScopeAgent::handleOpenPeakList(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	PeakList* pl = Dlg::selectPeakList( MW, d_pro );
	if( pl == 0 )
		return;
	setPeakList( pl );
}

void FourDScopeAgent::handleSavePeakList(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl && d_pl->getId() == 0 );

	savePeakList();
}

void FourDScopeAgent::handleMapPeakList(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl );

	RotateDlg dlg( MW, "Peaklist", "Spectrum" );
	Rotation rot( d_pl->getDimCount() );
	Spectrum* spec = d_spec2D;
	if( d_spec4D && d_show4DPlane )
		spec = d_spec4D;
	for( Dimension d = 0; d < d_pl->getDimCount(); d++ )
	{
		dlg.addDimension( d_pl->getAtomType( d ).getIsoLabel(), 
		( d < spec->getDimCount() )?spec->getColor( d ).getIsoLabel():"" );
		rot[ d ] = d;
	}

	if( dlg.rotate( rot ) )
		d_pl->setRotation( rot );
}

void FourDScopeAgent::handlePickPlPeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl );

	assert( d_pl );
	PeakSpace::PeakData pd;
	PeakModel::Params pp;
	pd.d_pos[ DimX ] = d_cursor[ DimX ];
	pd.d_pos[ DimY ] = d_cursor[ DimY ];
	pd.d_pos[ DimZ ] = d_cursor[ DimZ ];
    pd.d_pos[ DimW ] = d_cursor[ DimW ];
	Spectrum* spec = d_spec2D;
	if( d_spec4D && d_show4DPlane )
		spec = d_spec4D;
	PpmPoint p;
	p.assign( spec->getDimCount(), 0 );
	for( int i = 0; i < p.size(); i++ )
		p[i] = d_cursor[i];
	pd.d_amp = spec->getAt( p, true, d_folding ); 
	try
	{
		COP cop( true, true );
		d_pl->addPeak( pd, spec, cop );
		cop.d_done->registerForUndo( this );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Pick Peak", e.what() );
	}
}

void FourDScopeAgent::handleMovePlPeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl && d_plane.d_peaks->getSel().size() == 1 );

	Root::Index peak = *d_plane.d_peaks->getSel().begin();
	PeakPos pos;
	PeakModel::Params pp;
	pos[ DimX ] = d_cursor[ DimX ];
	pos[ DimY ] = d_cursor[ DimY ];
	pos[ DimZ ] = d_cursor[ DimZ ];
    pos[ DimW ] = d_cursor[ DimW ];
	Spectrum* spec = d_spec2D;
	if( d_spec4D && d_show4DPlane )
		spec = d_spec4D;
	PpmPoint p;
	p.assign( spec->getDimCount(), 0 );
	for( int i = 0; i < p.size(); i++ )
		p[i] = d_cursor[i];
	Amplitude amp = spec->getAt( p, true, d_folding ); 
	try
	{
		COP cop( true, true );
		d_pl->setPos( peak, pos, amp, 0, cop );
		cop.d_done->registerForUndo( this );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Move Peak", e.what() );
	}
}

void FourDScopeAgent::handleMovePlAlias(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl && d_plane.d_peaks->getSel().size() == 1 );

	Root::Index peak = *d_plane.d_peaks->getSel().begin();
	PeakPos pos;
	PeakModel::Params pp;
	pos[ DimX ] = d_cursor[ DimX ];
	pos[ DimY ] = d_cursor[ DimY ];
	pos[ DimZ ] = d_cursor[ DimZ ];
    pos[ DimW ] = d_cursor[ DimW ];
	Spectrum* spec = d_spec2D;
	if( d_spec4D && d_show4DPlane )
		spec = d_spec4D;
	PpmPoint p;
	p.assign( spec->getDimCount(), 0 );
	for( int i = 0; i < p.size(); i++ )
		p[i] = d_cursor[i];
	Amplitude amp = spec->getAt( p, true, d_folding ); 
	try
	{
		COP cop( true, true );
		d_pl->setPos( peak, pos, amp, spec, cop );
		cop.d_done->registerForUndo( this );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Move Peak Alias", e.what() );
	}
}

void FourDScopeAgent::handleLabelPlPeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl && d_plane.d_peaks->getSel().size() == 1 );

	Root::Index peak = ( *d_plane.d_peaks->getSel().begin() );
	bool ok	= FALSE;
	QString res	= QInputDialog::getText( "Set Peak Label", 
		"Please	enter a label:", QLineEdit::Normal, d_pl->getTag( peak ).data(), &ok, MW );
	if( !ok )
		return;
	try
	{
		COP cop( true, true );
		d_pl->setTag( peak, res.toLatin1(), cop );
		cop.d_done->registerForUndo( this );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Label Peak", e.what() );
	}
}

void FourDScopeAgent::handleDeletePlPeaks(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl && !d_plane.d_peaks->getSel().empty() );

	Root::Ref<Root::MakroTransaction> cmd = new Root::MakroTransaction( "Delete Peaks" );
	try
	{
		PeakPlaneView::Selection sel = d_plane.d_peaks->getSel();
		PeakPlaneView::Selection::const_iterator p;
		for( p = sel.begin(); p != sel.end(); ++p )
		{
			COP cop( true, true );
			d_pl->removePeak( (*p), cop );
			cmd->add( cop.d_done );
		}
	}catch( Root::Exception& e )
	{
		cmd->unexecute();
		Root::ReportToUser::alert( this, "Delete Peaks", e.what() );
		return;
	}
	cmd->registerForUndo( this );
}

void FourDScopeAgent::handleEditPlPeakAtts(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl && d_plane.d_peaks->getSel().size() == 1 );

	Root::Index id = ( *d_plane.d_peaks->getSel().begin() );
	Peak* peak = d_pl->getPeakList()->getPeak( id );
	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keyPeak ), peak );
}

void FourDScopeAgent::handleSetPlColor(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	QColor clr = QColorDialog::getColor( d_plane.d_peaks->getColor(), 
		MW );
	if( clr.isValid() )
	{
		d_plane.d_peaks->setColor( ( clr ) );
		d_plane.d_viewer->redraw();
        d_ortho.d_peaks->setColor( d_plane.d_peaks->getColor() );
        d_ortho.d_viewer->redraw();
    }
}

void FourDScopeAgent::handleDeleteAliasPeak(Action & a)
{
	ACTION_ENABLED_IF( a, !d_plane.d_tuples->getSel().empty() );

	SpinPointView::Selection sel = d_plane.d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Spins" );
	std::set<Spin*> test;
	for( p = sel.begin(); p != sel.end(); ++p )
	{
		if( test.count( (*p)[ DimX ] ) == 0 )
		{
			cmd->add( new MoveSpinCmd( d_pro->getSpins(), (*p)[ DimX ], 
			(*p)[ DimX ]->getShift(), // auf Home schieben l�cht Alias
			getSpec() ) );
			test.insert( (*p)[ DimX ] );
		}
		if( test.count( (*p)[ DimY ] ) == 0 )
		{
			cmd->add( new MoveSpinCmd( d_pro->getSpins(), (*p)[ DimY ], 
			(*p)[ DimY ]->getShift(), // auf Home schieben l�cht Alias
			getSpec() ) );
			test.insert( (*p)[ DimY ] );
		}
	}
	cmd->handle( this );
}

void FourDScopeAgent::handleFitWindowX(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	d_plane.d_ol[0].d_buf->fitToDim( DimX );
	d_plane.d_viewer->damageMe();
	if( !d_slices.empty() )
	{
		d_slices[ DimX ].d_buf2D->fitToArea();
		d_slices[ DimX ].d_viewer->redraw();
	}
}

void FourDScopeAgent::handleFitWindowY(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	d_plane.d_ol[0].d_buf->fitToDim( DimY );
	d_plane.d_viewer->damageMe();
	if( !d_slices.empty() )
	{
		d_slices[ DimY ].d_buf2D->fitToArea();
		d_slices[ DimY ].d_viewer->redraw();
	}
}

void FourDScopeAgent::handleGotoPlPeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl );

	Root::Index id;
	if( a.getParam( 0 ).isNull() )
	{
		bool ok	= FALSE;
		id	= QInputDialog::getInteger( "Goto Peak", 
			"Please	enter peak id:", 
			0, -999999, 999999, 1, &ok, MW );
		if( !ok )
			return;
	}else
		id = a.getParam( 0 ).getLong();

	try
	{
		PeakPos pos;
		d_pl->getPos( id, pos, (d_spec4D && d_show4DPlane)?d_spec4D:d_spec2D );
		PpmPoint p;
		p.assign( Root::Math::min( d_pl->getDimCount(), (d_show4DPlane)?4:2 ), 0 );
		for( Dimension d = 0; d < p.size(); d++ )
			p[d] = pos[d];
		setCursor( p );
        centerToCursor( p.size() > 2 );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Goto Peak", e.what() );
	}
}

void FourDScopeAgent::handleViewPlLabels(Action & a)
{
	if( a.getParamCount() == 0 )
		return;

	PeakPlaneView::Label q = (PeakPlaneView::Label) a.getParam( 0 ).getShort();
	if( q < PeakPlaneView::NONE || q >= PeakPlaneView::END )
		return;

	ACTION_CHECKED_IF( a, true,
		d_plane.d_peaks->getLabel() == q );
	
	d_plane.d_peaks->setLabel( q );
    d_ortho.d_peaks->setLabel( q );
    d_ortho.d_viewer->redraw();
	d_plane.d_viewer->redraw();
}

void FourDScopeAgent::handleSyncDepth(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_syncDepth );
	
	d_syncDepth = !d_syncDepth;
	// Unterfeature von SyncCursor
}

void FourDScopeAgent::handleAdjustIntensity(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	
	Dlg::adjustIntensity( MW, d_plane.d_intens );
}

