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

#include "PolyScopeAgent2.h"
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
using namespace Spec;

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
static QColor g_clrSlice3D = Qt::cyan;
static QColor g_clrPeak = Qt::yellow;

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr PolyScopeAgent2::SetResolution = "SetResolution";
Root::Action::CmdStr PolyScopeAgent2::ShowLowRes = "ShowLowRes";
Root::Action::CmdStr PolyScopeAgent2::Forward = "Forward";
Root::Action::CmdStr PolyScopeAgent2::Backward = "Backward";
Root::Action::CmdStr PolyScopeAgent2::FitWindow = "FitWindow";
Root::Action::CmdStr PolyScopeAgent2::ShowFolded = "ShowFolded";
Root::Action::CmdStr PolyScopeAgent2::SpecCalibrate = "SpecCalibrate";
Root::Action::CmdStr PolyScopeAgent2::AutoCenter = "AutoCenter";
Root::Action::CmdStr PolyScopeAgent2::ShowContour = "ShowContour";
Root::Action::CmdStr PolyScopeAgent2::ShowIntensity = "ShowIntensity";
Root::Action::CmdStr PolyScopeAgent2::AutoContour = "AutoContour";
Root::Action::CmdStr PolyScopeAgent2::ContourParams = "ContourParams";
Root::Action::CmdStr PolyScopeAgent2::PickSystem = "PickSystem";
Root::Action::CmdStr PolyScopeAgent2::PickHori = "PickHori";
Root::Action::CmdStr PolyScopeAgent2::PickVerti = "PickVerti";
Root::Action::CmdStr PolyScopeAgent2::MovePeak = "MovePeak";
Root::Action::CmdStr PolyScopeAgent2::MovePeakAlias = "MovePeakAlias";
Root::Action::CmdStr PolyScopeAgent2::LabelPeak = "LabelPeak";
Root::Action::CmdStr PolyScopeAgent2::DeletePeak = "DeletePeak";
Root::Action::CmdStr PolyScopeAgent2::DeleteSpinX = "DeleteSpinX";
Root::Action::CmdStr PolyScopeAgent2::DeleteSpinY = "DeleteSpinY";
Root::Action::CmdStr PolyScopeAgent2::HidePeak = "HidePeak";
Root::Action::CmdStr PolyScopeAgent2::ShowAllPeaks = "ShowAllPeaks";
Root::Action::CmdStr PolyScopeAgent2::ShowAlignment = "ShowAlignment";
Root::Action::CmdStr PolyScopeAgent2::Assign = "Assign";
Root::Action::CmdStr PolyScopeAgent2::Unassign = "Unassign";
Root::Action::CmdStr PolyScopeAgent2::SetSystemType = "SetSystemType";
Root::Action::CmdStr PolyScopeAgent2::ViewLabels = "ViewLabels";
Root::Action::CmdStr PolyScopeAgent2::SelectSpec2D = "SelectSpec2D";
Root::Action::CmdStr PolyScopeAgent2::LinkSystems = "LinkSystems";
Root::Action::CmdStr PolyScopeAgent2::UnlinkPred = "UnlinkPred";
Root::Action::CmdStr PolyScopeAgent2::UnlinkSucc = "UnlinkSucc";
Root::Action::CmdStr PolyScopeAgent2::SelectSystems = "SelectSystems";
Root::Action::CmdStr PolyScopeAgent2::SelectSpins = "SelectSpins";
Root::Action::CmdStr PolyScopeAgent2::HoldReference = "HoldReference";
Root::Action::CmdStr PolyScopeAgent2::UnhidePeak = "UnhidePeak";
Root::Action::CmdStr PolyScopeAgent2::CreateReport = "CreateReport";
Root::Action::CmdStr PolyScopeAgent2::AddRulerVerti = "AddRulerVerti";
Root::Action::CmdStr PolyScopeAgent2::AddRulerHori = "AddRulerHori";
Root::Action::CmdStr PolyScopeAgent2::RemoveRulers = "RemoveRulers";
Root::Action::CmdStr PolyScopeAgent2::RemoveAllRulers = "RemoveAllRulers";
Root::Action::CmdStr PolyScopeAgent2::AutoRuler = "AutoRuler";
Root::Action::CmdStr PolyScopeAgent2::ProposeHori = "ProposeHori";
Root::Action::CmdStr PolyScopeAgent2::ProposeVerti = "ProposeVerti";
Root::Action::CmdStr PolyScopeAgent2::ProposePeak = "ProposePeak";
Root::Action::CmdStr PolyScopeAgent2::SelectSpec3D = "SelectSpec3D";
Root::Action::CmdStr PolyScopeAgent2::PickSpin3D = "PickSpin3D";
Root::Action::CmdStr PolyScopeAgent2::MoveSpin3D = "MoveSpin3D";
Root::Action::CmdStr PolyScopeAgent2::MoveSpinAlias3D = "MoveSpinAlias3D";
Root::Action::CmdStr PolyScopeAgent2::DeleteSpins3D = "DeleteSpins3D";
Root::Action::CmdStr PolyScopeAgent2::LabelSpin3D = "LabelSpin3D";
Root::Action::CmdStr PolyScopeAgent2::ForceLabelSpin3D = "ForceLabelSpin3D";
Root::Action::CmdStr PolyScopeAgent2::SetWidth = "SetWidth";
Root::Action::CmdStr PolyScopeAgent2::FitWindow3D = "FitWindow3D";
Root::Action::CmdStr PolyScopeAgent2::AutoContour2 = "AutoContour2";
Root::Action::CmdStr PolyScopeAgent2::ContourParams2 = "ContourParams2";
Root::Action::CmdStr PolyScopeAgent2::ShowWithOff = "ShowWithOff";
Root::Action::CmdStr PolyScopeAgent2::Show3dPlane = "Show3dPlane";
Root::Action::CmdStr PolyScopeAgent2::AutoHide = "AutoHide";
Root::Action::CmdStr PolyScopeAgent2::StripCalibrate = "StripCalibrate";
Root::Action::CmdStr PolyScopeAgent2::ProposeSpin = "ProposeSpin";
Root::Action::CmdStr PolyScopeAgent2::EditAttsSpinH = "EditAttsSpinH";
Root::Action::CmdStr PolyScopeAgent2::EditAttsSpinV = "EditAttsSpinV";
Root::Action::CmdStr PolyScopeAgent2::EditAttsLink = "EditAttsLink";
Root::Action::CmdStr PolyScopeAgent2::EditAttsSysH = "EditAttsSysH";
Root::Action::CmdStr PolyScopeAgent2::EditAttsSysV = "EditAttsSysV";
Root::Action::CmdStr PolyScopeAgent2::EditAttsSpin3D = "EditAttsSpin3D";
Root::Action::CmdStr PolyScopeAgent2::CursorSync = "CursorSync";
Root::Action::CmdStr PolyScopeAgent2::GotoSystem = "GotoSystem";
Root::Action::CmdStr PolyScopeAgent2::NextSpec3D = "NextSpec3D";
Root::Action::CmdStr PolyScopeAgent2::PrevSpec3D = "PrevSpec3D";
Root::Action::CmdStr PolyScopeAgent2::NextSpec2D = "NextSpec2D";
Root::Action::CmdStr PolyScopeAgent2::PrevSpec2D = "PrevSpec2D";
Root::Action::CmdStr PolyScopeAgent2::ForwardPlane = "ForwardPlane";
Root::Action::CmdStr PolyScopeAgent2::BackwardPlane = "BackwardPlane";
Root::Action::CmdStr PolyScopeAgent2::ShowWithOff2 = "ShowWithOff2";
Root::Action::CmdStr PolyScopeAgent2::ShowLinks = "ShowLinks";
Root::Action::CmdStr PolyScopeAgent2::DeleteLinks = "DeleteLinks";
Root::Action::CmdStr PolyScopeAgent2::LabelVerti = "LabelVerti";
Root::Action::CmdStr PolyScopeAgent2::LabelHori = "LabelHori";
Root::Action::CmdStr PolyScopeAgent2::SetCandidates = "SetCandidates";
Root::Action::CmdStr PolyScopeAgent2::ShowInfered = "ShowInfered";
Root::Action::CmdStr PolyScopeAgent2::ShowUnlabeled = "ShowUnlabeled";
Root::Action::CmdStr PolyScopeAgent2::CreateLinks = "CreateLinks";
Root::Action::CmdStr PolyScopeAgent2::ForceCross = "ForceCross";
Root::Action::CmdStr PolyScopeAgent2::DeleteLinks3D = "DeleteLinks3D";
Root::Action::CmdStr PolyScopeAgent2::ViewLabels3D = "ViewLabels3D";
Root::Action::CmdStr PolyScopeAgent2::AutoGain = "AutoGain";
Root::Action::CmdStr PolyScopeAgent2::AutoGain3D = "AutoGain3D";
Root::Action::CmdStr PolyScopeAgent2::ShowGhosts = "ShowGhosts";
Root::Action::CmdStr PolyScopeAgent2::AutoHold = "AutoHold";
Root::Action::CmdStr PolyScopeAgent2::PickLabel3D = "PickLabel3D";
Root::Action::CmdStr PolyScopeAgent2::ShowLinks2 = "ShowLinks2";
Root::Action::CmdStr PolyScopeAgent2::ShowInfered2 = "ShowInfered2";
Root::Action::CmdStr PolyScopeAgent2::ShowUnlabeled2 = "ShowUnlabeled2";
Root::Action::CmdStr PolyScopeAgent2::SetDepth = "SetDepth";
Root::Action::CmdStr PolyScopeAgent2::GhostLabels = "GhostLabels";
Root::Action::CmdStr PolyScopeAgent2::HidePeak2 = "HidePeak2";
Root::Action::CmdStr PolyScopeAgent2::GotoPeak = "GotoPeak";
Root::Action::CmdStr PolyScopeAgent2::RangeSync = "RangeSync";
Root::Action::CmdStr PolyScopeAgent2::EditAttsSys3D = "EditAttsSys3D";
Root::Action::CmdStr PolyScopeAgent2::EditAttsLink3D = "EditAttsLink3D";
Root::Action::CmdStr PolyScopeAgent2::OverlayCount = "OverlayCount";
Root::Action::CmdStr PolyScopeAgent2::ActiveOverlay = "ActiveOverlay";
Root::Action::CmdStr PolyScopeAgent2::SetPosColor = "SetPosColor";
Root::Action::CmdStr PolyScopeAgent2::SetNegColor = "SetNegColor";
Root::Action::CmdStr PolyScopeAgent2::OverlaySpec = "OverlaySpec";
Root::Action::CmdStr PolyScopeAgent2::CntFactor = "CntFactor";
Root::Action::CmdStr PolyScopeAgent2::CntThreshold = "CntThreshold";
Root::Action::CmdStr PolyScopeAgent2::CntOption = "CntOption";
Root::Action::CmdStr PolyScopeAgent2::AddLayer = "AddLayer";
Root::Action::CmdStr PolyScopeAgent2::ComposeLayers = "ComposeLayers";
Root::Action::CmdStr PolyScopeAgent2::UseLinkColors = "UseLinkColors";
Root::Action::CmdStr PolyScopeAgent2::UseLinkColors3D = "UseLinkColors3D";
Root::Action::CmdStr PolyScopeAgent2::SetLinkParams = "SetLinkParams";
Root::Action::CmdStr PolyScopeAgent2::SetLinkParams3D = "SetLinkParams3D";
Root::Action::CmdStr PolyScopeAgent2::GotoPoint = "GotoPoint";
Root::Action::CmdStr PolyScopeAgent2::NewPeakList = "NewPeakList";
Root::Action::CmdStr PolyScopeAgent2::OpenPeakList = "OpenPeakList";
Root::Action::CmdStr PolyScopeAgent2::SavePeakList = "SavePeakList";
Root::Action::CmdStr PolyScopeAgent2::MapPeakList = "MapPeakList";
Root::Action::CmdStr PolyScopeAgent2::PickPlPeak = "PickPlPeak";
Root::Action::CmdStr PolyScopeAgent2::MovePlPeak = "MovePlPeak";
Root::Action::CmdStr PolyScopeAgent2::MovePlAlias = "MovePlAlias";
Root::Action::CmdStr PolyScopeAgent2::LabelPlPeak = "LabelPlPeak";
Root::Action::CmdStr PolyScopeAgent2::DeletePlPeaks = "DeletePlPeaks";
Root::Action::CmdStr PolyScopeAgent2::EditPlPeakAtts = "EditPlPeakAtts";
Root::Action::CmdStr PolyScopeAgent2::SetPlColor = "SetPlColor";
Root::Action::CmdStr PolyScopeAgent2::DeleteAliasPeak = "DeleteAliasPeak";
Root::Action::CmdStr PolyScopeAgent2::FitWindowX = "FitWindowX";
Root::Action::CmdStr PolyScopeAgent2::FitWindowY = "FitWindowY";
Root::Action::CmdStr PolyScopeAgent2::GotoPlPeak = "GotoPlPeak";
Root::Action::CmdStr PolyScopeAgent2::ViewPlLabels = "ViewPlLabels";
Root::Action::CmdStr PolyScopeAgent2::SyncDepth = "SyncDepth";
Root::Action::CmdStr PolyScopeAgent2::AdjustIntensity = "AdjustIntensity";
Root::Action::CmdStr PolyScopeAgent2::ShowUnknown = "ShowUnknown";
Root::Action::CmdStr PolyScopeAgent2::ShowUnknown2 = "ShowUnknown2";

ACTION_SLOTS_BEGIN( PolyScopeAgent2 )
    { PolyScopeAgent2::ShowUnknown, &PolyScopeAgent2::handleShowUnknown },
    { PolyScopeAgent2::ShowUnknown2, &PolyScopeAgent2::handleShowUnknown2 },
    { PolyScopeAgent2::AdjustIntensity, &PolyScopeAgent2::handleAdjustIntensity },
    { PolyScopeAgent2::SyncDepth, &PolyScopeAgent2::handleSyncDepth },
    { PolyScopeAgent2::ViewPlLabels, &PolyScopeAgent2::handleViewPlLabels },
    { PolyScopeAgent2::GotoPlPeak, &PolyScopeAgent2::handleGotoPlPeak },
    { PolyScopeAgent2::FitWindowX, &PolyScopeAgent2::handleFitWindowX },
    { PolyScopeAgent2::FitWindowY, &PolyScopeAgent2::handleFitWindowY },
    { PolyScopeAgent2::DeleteAliasPeak, &PolyScopeAgent2::handleDeleteAliasPeak },
    { PolyScopeAgent2::MovePlAlias, &PolyScopeAgent2::handleMovePlAlias },
    { PolyScopeAgent2::SetPlColor, &PolyScopeAgent2::handleSetPlColor },
    { PolyScopeAgent2::NewPeakList, &PolyScopeAgent2::handleNewPeakList },
    { PolyScopeAgent2::OpenPeakList, &PolyScopeAgent2::handleOpenPeakList },
    { PolyScopeAgent2::SavePeakList, &PolyScopeAgent2::handleSavePeakList },
    { PolyScopeAgent2::MapPeakList, &PolyScopeAgent2::handleMapPeakList },
    { PolyScopeAgent2::PickPlPeak, &PolyScopeAgent2::handlePickPlPeak },
    { PolyScopeAgent2::MovePlPeak, &PolyScopeAgent2::handleMovePlPeak },
    { PolyScopeAgent2::LabelPlPeak, &PolyScopeAgent2::handleLabelPlPeak },
    { PolyScopeAgent2::DeletePlPeaks, &PolyScopeAgent2::handleDeletePlPeaks },
    { PolyScopeAgent2::EditPlPeakAtts, &PolyScopeAgent2::handleEditPlPeakAtts },
    { PolyScopeAgent2::GotoPoint, &PolyScopeAgent2::handleGotoPoint },
    { PolyScopeAgent2::SetLinkParams, &PolyScopeAgent2::handleSetLinkParams },
    { PolyScopeAgent2::SetLinkParams3D, &PolyScopeAgent2::handleSetLinkParams3D },
    { PolyScopeAgent2::UseLinkColors, &PolyScopeAgent2::handleUseLinkColors },
    { PolyScopeAgent2::UseLinkColors3D, &PolyScopeAgent2::handleUseLinkColors3D },
    { PolyScopeAgent2::ComposeLayers, &PolyScopeAgent2::handleComposeLayers },
    { PolyScopeAgent2::AddLayer, &PolyScopeAgent2::handleAddLayer },
    { PolyScopeAgent2::CntFactor, &PolyScopeAgent2::handleCntFactor },
    { PolyScopeAgent2::CntThreshold, &PolyScopeAgent2::handleCntThreshold },
    { PolyScopeAgent2::CntOption, &PolyScopeAgent2::handleCntOption },
    { PolyScopeAgent2::OverlaySpec, &PolyScopeAgent2::handleOverlaySpec },
    { PolyScopeAgent2::OverlayCount, &PolyScopeAgent2::handleOverlayCount },
    { PolyScopeAgent2::ActiveOverlay, &PolyScopeAgent2::handleActiveOverlay },
    { PolyScopeAgent2::SetPosColor, &PolyScopeAgent2::handleSetPosColor },
    { PolyScopeAgent2::SetNegColor, &PolyScopeAgent2::handleSetNegColor },
    { PolyScopeAgent2::EditAttsSys3D, &PolyScopeAgent2::handleEditAttsSys3D },
    { PolyScopeAgent2::EditAttsLink3D, &PolyScopeAgent2::handleEditAttsLink3D },
    { PolyScopeAgent2::RangeSync, &PolyScopeAgent2::handleRangeSync },
    { PolyScopeAgent2::GotoPeak, &PolyScopeAgent2::handleGotoPeak },
    { PolyScopeAgent2::HidePeak2, &PolyScopeAgent2::handleHidePeak2 },
    { PolyScopeAgent2::GhostLabels, &PolyScopeAgent2::handleGhostLabels },
    { PolyScopeAgent2::SetDepth, &PolyScopeAgent2::handleSetDepth },
    { PolyScopeAgent2::ShowLinks2, &PolyScopeAgent2::handleShowLinks2 },
    { PolyScopeAgent2::ShowInfered2, &PolyScopeAgent2::handleShowInfered2 },
    { PolyScopeAgent2::PickLabel3D, &PolyScopeAgent2::handlePickLabel3D },
    { PolyScopeAgent2::AutoHold, &PolyScopeAgent2::handleAutoHold },
    { PolyScopeAgent2::ShowGhosts, &PolyScopeAgent2::handleShowGhosts },
    { PolyScopeAgent2::AutoGain3D, &PolyScopeAgent2::handleAutoGain3D },
    { PolyScopeAgent2::AutoGain, &PolyScopeAgent2::handleAutoGain },
    { PolyScopeAgent2::ViewLabels3D, &PolyScopeAgent2::handleViewLabels3D },
    { PolyScopeAgent2::DeleteLinks3D, &PolyScopeAgent2::handleDeleteLinks3D },
    { PolyScopeAgent2::ForceCross, &PolyScopeAgent2::handleForceCross },
    { PolyScopeAgent2::ShowUnlabeled2, &PolyScopeAgent2::handleShowUnlabeled2 },
    { PolyScopeAgent2::ShowUnlabeled, &PolyScopeAgent2::handleShowUnlabeled },
    { PolyScopeAgent2::CreateLinks, &PolyScopeAgent2::handleCreateLinks },
    { PolyScopeAgent2::ShowInfered, &PolyScopeAgent2::handleShowInfered },
    { PolyScopeAgent2::SetCandidates, &PolyScopeAgent2::handleSetCandidates },
    { PolyScopeAgent2::LabelVerti, &PolyScopeAgent2::handleLabelVerti },
    { PolyScopeAgent2::LabelHori, &PolyScopeAgent2::handleLabelHori },
    { PolyScopeAgent2::DeleteLinks, &PolyScopeAgent2::handleDeleteLinks },
    { PolyScopeAgent2::ShowLinks, &PolyScopeAgent2::handleShowLinks },
    { PolyScopeAgent2::ForwardPlane, &PolyScopeAgent2::handleForwardPlane },
    { PolyScopeAgent2::BackwardPlane, &PolyScopeAgent2::handleBackwardPlane },
    { PolyScopeAgent2::ShowWithOff2, &PolyScopeAgent2::handleShowWithOff2 },
    { PolyScopeAgent2::NextSpec3D, &PolyScopeAgent2::handleNextSpec3D },
    { PolyScopeAgent2::PrevSpec3D, &PolyScopeAgent2::handlePrevSpec3D },
    { PolyScopeAgent2::NextSpec2D, &PolyScopeAgent2::handleNextSpec2D },
    { PolyScopeAgent2::PrevSpec2D, &PolyScopeAgent2::handlePrevSpec2D },
    { PolyScopeAgent2::GotoSystem, &PolyScopeAgent2::handleGotoSystem },
    { PolyScopeAgent2::CursorSync, &PolyScopeAgent2::handleCursorSync },
    { PolyScopeAgent2::EditAttsSpinH, &PolyScopeAgent2::handleEditAttsSpinH },
    { PolyScopeAgent2::EditAttsSpinV, &PolyScopeAgent2::handleEditAttsSpinV },
    { PolyScopeAgent2::EditAttsLink, &PolyScopeAgent2::handleEditAttsLink },
    { PolyScopeAgent2::EditAttsSysH, &PolyScopeAgent2::handleEditAttsSysH },
    { PolyScopeAgent2::EditAttsSysV, &PolyScopeAgent2::handleEditAttsSysV },
    { PolyScopeAgent2::EditAttsSpin3D, &PolyScopeAgent2::handleEditAttsSpin3D },
    { PolyScopeAgent2::ProposeSpin, &PolyScopeAgent2::handleProposeSpin },
    { PolyScopeAgent2::Show3dPlane, &PolyScopeAgent2::handleShow3dPlane },
    { PolyScopeAgent2::AutoHide, &PolyScopeAgent2::handleAutoHide },
    { PolyScopeAgent2::StripCalibrate, &PolyScopeAgent2::handleStripCalibrate },
    { PolyScopeAgent2::ShowWithOff, &PolyScopeAgent2::handleShowWithOff },
    { PolyScopeAgent2::AutoContour2, &PolyScopeAgent2::handleAutoContour2 },
    { PolyScopeAgent2::ContourParams2, &PolyScopeAgent2::handleContourParams2 },
    { PolyScopeAgent2::SetWidth, &PolyScopeAgent2::handleSetWidth },
    { PolyScopeAgent2::FitWindow3D, &PolyScopeAgent2::handleFitWindow3D },
    { PolyScopeAgent2::PickSpin3D, &PolyScopeAgent2::handlePickSpin3D },
    { PolyScopeAgent2::MoveSpin3D, &PolyScopeAgent2::handleMoveSpin3D },
    { PolyScopeAgent2::MoveSpinAlias3D, &PolyScopeAgent2::handleMoveSpinAlias3D },
    { PolyScopeAgent2::DeleteSpins3D, &PolyScopeAgent2::handleDeleteSpins3D },
    { PolyScopeAgent2::LabelSpin3D, &PolyScopeAgent2::handleLabelSpin3D },
    { PolyScopeAgent2::ForceLabelSpin3D, &PolyScopeAgent2::handleForceLabelSpin3D },
    { PolyScopeAgent2::SelectSpec3D, &PolyScopeAgent2::handleSelectSpec3D },
    { PolyScopeAgent2::ProposeHori, &PolyScopeAgent2::handleProposeHori },
    { PolyScopeAgent2::ProposeVerti, &PolyScopeAgent2::handleProposeVerti },
    { PolyScopeAgent2::ProposePeak, &PolyScopeAgent2::handleProposePeak },
    { PolyScopeAgent2::AutoRuler, &PolyScopeAgent2::handleAutoRuler },
    { PolyScopeAgent2::AddRulerVerti, &PolyScopeAgent2::handleAddRulerVerti },
    { PolyScopeAgent2::AddRulerHori, &PolyScopeAgent2::handleAddRulerHori },
    { PolyScopeAgent2::RemoveRulers, &PolyScopeAgent2::handleRemoveRulers },
    { PolyScopeAgent2::RemoveAllRulers, &PolyScopeAgent2::handleRemoveAllRulers },
    { PolyScopeAgent2::CreateReport, &PolyScopeAgent2::handleCreateReport },
    { PolyScopeAgent2::UnhidePeak, &PolyScopeAgent2::handleUnhidePeak },
    { PolyScopeAgent2::HoldReference, &PolyScopeAgent2::handleHoldReference },
    { PolyScopeAgent2::ShowAllPeaks, &PolyScopeAgent2::handleShowAllPeaks },
    { PolyScopeAgent2::ShowAlignment, &PolyScopeAgent2::handleShowAlignment },
    { PolyScopeAgent2::Assign, &PolyScopeAgent2::handleAssign },
    { PolyScopeAgent2::Unassign, &PolyScopeAgent2::handleUnassign },
    { PolyScopeAgent2::SetSystemType, &PolyScopeAgent2::handleSetSystemType },
    { PolyScopeAgent2::ViewLabels, &PolyScopeAgent2::handleViewLabels },
    { PolyScopeAgent2::SelectSpec2D, &PolyScopeAgent2::handleSelectSpec2D },
    { PolyScopeAgent2::LinkSystems, &PolyScopeAgent2::handleLinkSystems },
    { PolyScopeAgent2::UnlinkPred, &PolyScopeAgent2::handleUnlinkPred },
    { PolyScopeAgent2::UnlinkSucc, &PolyScopeAgent2::handleUnlinkSucc },
    { PolyScopeAgent2::DeletePeak, &PolyScopeAgent2::handleDeletePeak },
    { PolyScopeAgent2::DeleteSpinX, &PolyScopeAgent2::handleDeleteSpinX },
    { PolyScopeAgent2::DeleteSpinY, &PolyScopeAgent2::handleDeleteSpinY },
    { PolyScopeAgent2::LabelPeak, &PolyScopeAgent2::handleLabelPeak },
    { PolyScopeAgent2::HidePeak, &PolyScopeAgent2::handleHidePeak },
    { PolyScopeAgent2::MovePeakAlias, &PolyScopeAgent2::handleMovePeakAlias },
    { PolyScopeAgent2::MovePeak, &PolyScopeAgent2::handleMovePeak },
    { PolyScopeAgent2::PickVerti, &PolyScopeAgent2::handlePickVerti },
    { PolyScopeAgent2::PickHori, &PolyScopeAgent2::handlePickHori },
    { PolyScopeAgent2::PickSystem, &PolyScopeAgent2::handlePickSystem },
    { PolyScopeAgent2::ContourParams, &PolyScopeAgent2::handleContourParams },
    { PolyScopeAgent2::AutoContour, &PolyScopeAgent2::handleAutoContour },
    { PolyScopeAgent2::ShowIntensity, &PolyScopeAgent2::handleShowIntensity },
    { PolyScopeAgent2::ShowContour, &PolyScopeAgent2::handleShowContour },
    { PolyScopeAgent2::AutoCenter, &PolyScopeAgent2::handleAutoCenter },
    { PolyScopeAgent2::SpecCalibrate, &PolyScopeAgent2::handleSpecCalibrate },
    { PolyScopeAgent2::ShowFolded, &PolyScopeAgent2::handleShowFolded },
    { PolyScopeAgent2::FitWindow, &PolyScopeAgent2::handleFitWindow },
    { PolyScopeAgent2::Backward, &PolyScopeAgent2::handleBackward },
    { PolyScopeAgent2::Forward, &PolyScopeAgent2::handleForward },
    { PolyScopeAgent2::ShowLowRes, &PolyScopeAgent2::handleShowLowRes },
    { PolyScopeAgent2::SetResolution, &PolyScopeAgent2::handleSetResolution },
ACTION_SLOTS_END( PolyScopeAgent2 )

//////////////////////////////////////////////////////////////////////

#define MW ((Lexi::MainWindow*) getParent())->getQt()
static const Dimension s_defDim = 2;

PolyScopeAgent2::PolyScopeAgent2(Root::Agent* parent, Spectrum* spec, Project* pro, 
								 bool homo, bool synchro ):
	Root::Agent( parent ), d_lock( false ), d_autoRuler( false ), d_aol( 0 ),
		d_synchro( synchro ), d_popLabel( 0 ), d_popSpec2D( 0 ), d_pl(0),
		d_popSpec3D( 0 ), d_popStrip(0), d_popPlane(0)
{
	assert( spec && spec->getDimCount() >= 2 && spec->getDimCount() <= 3 );
	assert( pro );
	d_pro = pro;
	d_pro->addObserver( this );

	d_spec2D = spec;
	d_orig = spec;
	if( !d_synchro )
	{
		// Poly und HomoScope
		d_src2D = new SpinPointSpace( pro->getSpins(), 
			pro->getRepository()->getTypes(), 0, 
			true, true, false );
		d_src2D->setSpec( spec );
		d_src3D = new SpinPointSpace( pro->getSpins(), pro->getRepository()->getTypes(), 0, 
			true, true, false );
	}else
	{
		if( d_spec2D->getKeyLabel( DimX ).isNull() ||
			d_spec2D->getKeyLabel( DimY ).isNull() )
			throw Root::Exception( "Unique spin labels required along dimensions X and Y" );

		// SynchroScope
		// bool link, bool infer, bool nulls, bool offs, bool pathim, bool joker
		d_src2D = new SpinPointSpace( pro->getSpins(), 
			pro->getRepository()->getTypes(), 
			false, true, false, true, false );
		d_src2D->setSpec( spec );
		d_src3D = new SpinPointSpace( pro->getSpins(), pro->getRepository()->getTypes(), 
			false, true, true, true, false );
	}
	d_mdl2D = new LinkFilterRotSpace( d_src2D, spec );
	d_mdl3D = new LinkFilterRotSpace( d_src3D );

	initParams();
	if( !d_synchro )
		buildPopup();
	else
		buildPopup2();
	initViews( homo );
	if( d_spec2D->getDimCount() > 2 )
	{
		setSpec3D( d_spec2D );
		d_cursor[ DimZ ] = d_spec2D->getScale( DimZ ).getRange().first;
		d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
	}
	updateSpecPop2D();
	updateSpecPop3D();
}

PolyScopeAgent2::~PolyScopeAgent2()
{
	d_pro->removeObserver( this );
	d_pro = 0;
	
	if( d_popSpec2D )
		delete d_popSpec2D;
	if( d_popSpec3D )
		delete d_popSpec3D;
	if( d_popLabel )
		delete d_popLabel;
	if( d_popPlane )
		delete d_popPlane;
	if( d_popStrip )
		delete d_popStrip;

	GlobalCursor::removeObserver( this );
}

void PolyScopeAgent2::initViews( bool homo )
{
	d_lock = true;
	d_cursor.assign( 3, 0 );
	d_slices.assign( (homo)?2:3, SliceSocket() );

	createPlane();
	createSlice( DimX, DimX );
	d_slices[ DimX ].d_spec2D = new SpecProjector( d_spec2D, DimX );
	d_slices[ DimX ].d_buf2D->setSpectrum( d_slices[ DimX ].d_spec2D, false );
	createSlice( DimY, DimY );
	d_slices[ DimY ].d_spec2D = new SpecProjector( d_spec2D, DimY );
	d_slices[ DimY ].d_buf2D->setSpectrum( d_slices[ DimY ].d_spec2D, false );
	if( !homo )
	{
		d_strips.assign( 2, StripSocket() );
		createSlice( DimY, DimZ );
		createStrip( DimX );
		createStrip( DimY );
	}
	d_lock = false;

	// d_plane.d_ol[0].d_buf->fitToArea(); wird verz�ert gemacht
	updatePlaneLabel();
}

void PolyScopeAgent2::buildPopup()
{
	d_popSpec2D = new Gui::Menu();
	d_popSpec3D = new Gui::Menu();

	Gui::Menu* menuPeaks = new Gui::Menu( d_popPlane );
	Gui::Menu::item( menuPeaks, this, "&Pick Peak", PickPlPeak, false );
	Gui::Menu::item( menuPeaks, this, "&Move Peak", MovePlPeak, false );
	Gui::Menu::item( menuPeaks, this, "&Move Peak Alias", MovePlAlias, false );
	Gui::Menu::item( menuPeaks, this, "&Label Peak...", LabelPlPeak, false );
	Gui::Menu::item( menuPeaks, this, "&Delete Peaks", DeletePlPeaks, false );
	Gui::Menu::item( menuPeaks, this, "&Edit Attributes...", EditPlPeakAtts, false );
	menuPeaks->insertSeparator();
	Gui::Menu::item( menuPeaks, this, "&Open Peaklist...", OpenPeakList, false );

	d_popPlane = new Gui::Menu();
	Gui::Menu::item( d_popPlane, this, "Hold Reference", HoldReference, true );
	Gui::Menu::item( d_popPlane, this, "Add Vertical Ruler", AddRulerHori, false );
	Gui::Menu::item( d_popPlane, this, "Add Horizontal Ruler", AddRulerVerti, false );
	Gui::Menu::item( d_popPlane, this, "Remove All Rulers", RemoveAllRulers, false );
	d_popPlane->insertItem( "Select Spectrum", d_popSpec2D );
	d_popPlane->insertItem( "Peaks", menuPeaks );
	d_popPlane->insertSeparator();
	Gui::Menu::item( d_popPlane, this, "&Pick New System", PickSystem, false );
	Gui::Menu::item( d_popPlane, this, "Propose System...", ProposePeak, false );
	Gui::Menu::item( d_popPlane, this, "Extend Horizontally", PickHori, false );
	Gui::Menu::item( d_popPlane, this, "Propose Horizontally...", ProposeHori, false );
	Gui::Menu::item( d_popPlane, this, "Extend Vertically", PickVerti, false );
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
	Gui::Menu::item( d_popPlane, this, "Set Peak Depth...", SetDepth, false );
	Gui::Menu::item( d_popPlane, this, "Show Alignment...", ShowAlignment, false );
	Gui::Menu::item( d_popPlane, this, "Backward", Backward, false );
	Gui::Menu::item( d_popPlane, this, "Show 3D Plane", Show3dPlane, true );
	Gui::Menu::item( d_popPlane, this, "Fit Window", FitWindow, false );

	d_popStrip = new Gui::Menu();
	d_popStrip->insertItem( "Select Spectrum", d_popSpec3D );
	d_popStrip->insertItem( "Peaks", menuPeaks );
	d_popStrip->insertSeparator();
	Gui::Menu::item( d_popStrip, this, "&Pick Spin", PickSpin3D, false );
	Gui::Menu::item( d_popStrip, this, "Pick Label...", PickLabel3D, false );
	Gui::Menu::item( d_popStrip, this, "&Propose Spin...", ProposeSpin, false );
	Gui::Menu::item( d_popStrip, this, "&Move Spin", MoveSpin3D, false );
	Gui::Menu::item( d_popStrip, this, "&Move Spin Alias", MoveSpinAlias3D, false );
	Gui::Menu::item( d_popStrip, this, "Label Spin...", LabelSpin3D, false );
	Gui::Menu::item( d_popStrip, this, "&Force Spin Label", ForceLabelSpin3D, false );
	Gui::Menu::item( d_popStrip, this, "&Delete Spins", DeleteSpins3D, false );
	Gui::Menu::item( d_popStrip, this, "Delete Links", DeleteLinks3D, false );
	Gui::Menu::item( d_popStrip, this, "Set Link Params...", SetLinkParams3D, false );
	menuAtts = new Gui::Menu( d_popStrip );
	Gui::Menu::item( menuAtts, this, "Spin...", EditAttsSpin3D, false );
	Gui::Menu::item( menuAtts, this, "System...", EditAttsSys3D, false );
	Gui::Menu::item( menuAtts, this, "Spin Link...", EditAttsLink3D, false );
	d_popStrip->insertItem( "&Edit Attributes", menuAtts );
	d_popStrip->insertSeparator();
	Gui::Menu::item( d_popStrip, this, "Set Peak Width...", SetWidth, false );
	Gui::Menu::item( d_popStrip, this, "Show 3D Plane", Show3dPlane, true );
	Gui::Menu::item( d_popStrip, this, "Fit Window", FitWindow3D, false );
}

void PolyScopeAgent2::buildPopup2()
{
	d_popSpec3D = new Gui::Menu();
	d_popLabel = new Gui::Menu(); // Explizit lschen

	d_popPlane = new Gui::Menu();
	Gui::Menu::item( d_popPlane, this, "Pick System", PickSystem, false );
	Gui::Menu::item( d_popPlane, this, "Move System", MovePeak, false );
	Gui::Menu::item( d_popPlane, this, "Move System Alias", MovePeakAlias, false );
	Gui::Menu::item( d_popPlane, this, "Delete Systems", DeletePeak, false );
	Gui::Menu::item( d_popPlane, this, "Edit Attributes...", EditAttsSysH, false );
	/* TODO
	d_popPlane->insertSeparator();
	Gui::Menu::item( d_popPlane, this, "&Pick Peak", PickPeak, false );
	Gui::Menu::item( d_popPlane, this, "&Move Peak", MovePeak, false );
	Gui::Menu::item( d_popPlane, this, "&Force Peak Label", ForceLabelPeak, false );
	Gui::Menu::item( d_popPlane, this, "&Delete Peaks", DeletePeaks, false );
	Gui::Menu::item( d_popPlane, this, "Edit Peak Attributes...", EditPeakAtts, false );
	*/
	d_popPlane->insertSeparator();
	Gui::Menu::item( d_popPlane, this, "Calibrate from System", SpecCalibrate, false );
	Gui::Menu::item( d_popPlane, this, "Show 3D Plane", Show3dPlane, true );
	Gui::Menu::item( d_popPlane, this, "Fit Window", FitWindow, false );


	d_popStrip = new Gui::Menu();
	d_popStrip->insertItem( "Select Spectrum", d_popSpec3D );
	d_popStrip->insertSeparator();
	Gui::Menu::item( d_popStrip, this, "&Pick Spin", PickSpin3D, false );
	Gui::Menu::item( d_popStrip, this, "Pick Label...", PickLabel3D, false );
	Gui::Menu::item( d_popStrip, this, "&Move Spin", MoveSpin3D, false );
	Gui::Menu::item( d_popStrip, this, "&Move Spin Alias", MoveSpinAlias3D, false );
	d_popStrip->insertItem( "Label Spin", d_popLabel );
	Gui::Menu::item( d_popStrip, this, "&Force Spin Label", ForceLabelSpin3D, false );
	Gui::Menu::item( d_popStrip, this, "&Delete Spins", DeleteSpins3D, false );
	Gui::Menu::item( d_popStrip, this, "Edit Attributes...", EditAttsSpin3D, false );
	d_popStrip->insertSeparator();
	Gui::Menu::item( d_popStrip, this, "Set Peak Width...", SetWidth, false );
	Gui::Menu::item( d_popStrip, this, "Show 3D Plane", Show3dPlane, true );
	Gui::Menu::item( d_popStrip, this, "Fit Window", FitWindow3D, false );
}

void PolyScopeAgent2::updateSpecPop2D()
{
	if( d_popSpec2D == 0 )
		return;

	d_popSpec2D->purge();
	ColorMap a, b;
	d_spec2D->getColors( a );
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
	if( d_spec2D == d_spec3D )
	{
		Gui::Menu::item( d_popSpec2D, this, d_spec3D->getName(), 
			PolyScopeAgent2::SelectSpec2D, true )->addParam( Root::Any( d_spec3D ) );
	}
	Sort::const_iterator pp1;
	Project::SpecSet::const_iterator p1;
	for( p1 = l.begin(); p1 != l.end(); ++p1 )
		d_sort2D[ (*p1)->getName() ] = (*p1);
	for( pp1 = d_sort2D.begin(); pp1 != d_sort2D.end(); ++pp1 )
	{
		Gui::Menu::item( d_popSpec2D, this, (*pp1).first.data(), 
			PolyScopeAgent2::SelectSpec2D, 
			true )->addParam( Root::Any( (*pp1).second ) );
	}
}

void PolyScopeAgent2::updateSpecPop3D()
{
	d_popSpec3D->purge();
	ColorMap a, b;
	d_spec2D->getColors( a );
	Project::SpecSet l;
	Spectrum* spec = 0;
	if( a.size() == 3 )
		a[ DimZ ] = AtomType(); // Joker
	else
		a.push_back( AtomType() );

	if( d_synchro )
	{
		KeyLabels keys( 3 ); // 3, da letzte Dimension Don't care, aber 3D Spec gesucht.
		keys[ DimX ] = d_spec2D->getKeyLabel( DimX );
		keys[ DimY ] = d_spec2D->getKeyLabel( DimY );

		l = d_pro->findSpectra( keys );
	}else
	{
		const Project::SpectrumMap& sm = d_pro->getSpectra();
		Project::SpectrumMap::const_iterator p;
		Rotation rot;
		if( d_orig->getDimCount() == 3 )
			l.insert( d_orig );
		for( p = sm.begin(); p != sm.end(); ++p )
		{
			spec = (*p).second;
			if( spec->getDimCount() == 3 && spec->getId() != d_orig->getId() )
			{
				spec->getColors( b );
				if( a[ DimX ] == b[ DimX ] && a[ DimY ] == b[ DimY ] )
					l.insert( spec );
				else
				{
					// TEST qDebug( "id=%d name=%s", spec->getId(), spec->getName() );
					if( SpectrumType::autoRotate( a, spec, rot, false ) ) // Keine Aufl�ungsoptimierung
					{
						l.insert( new SpecRotator( spec, rot ) );
					}
				}
			}
		}
	}
	Project::SpecSet::const_iterator p1;
	Sort::const_iterator pp1;
	for( p1 = l.begin(); p1 != l.end(); ++p1 )
		d_sort3D[ (*p1)->getName() ] = (*p1);
	for( pp1 = d_sort3D.begin(); pp1 != d_sort3D.end(); ++pp1 )
	{
		Gui::Menu::item( d_popSpec3D, this, (*pp1).first.data(),  
			PolyScopeAgent2::SelectSpec3D, true )->addParam( Root::Any( (*pp1).second ) );
	}
}

void PolyScopeAgent2::createPlane()
{
	d_plane.d_ol.assign( 1, PlaneSocket::Overlay() );

	d_plane.d_viewer = new SpecViewer( new ViewAreaMdl( true, true, true, true ), VIEWCOUNT );
	d_plane.d_viewer->getViewArea()->addObserver( this );

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
	d_plane.d_mdl3D = new RangeFilterSpace( d_mdl3D, DimZ );
	d_plane.d_tuples = new SpinPointView( d_plane.d_viewer, d_mdl2D );
	d_plane.d_tuples->setLabel( SpinPointView::PairLabelSysOrResi );
	d_plane.d_viewer->getViews()->replace( TUPLES, d_plane.d_tuples );
	CursorView* cv = new CursorView( d_plane.d_viewer, d_plane.d_cur );
	d_plane.d_viewer->getViews()->replace( CURSORS, cv );
	if( d_folding )
		d_plane.d_viewer->getViews()->replace( FOLDING, new FoldingView( d_plane.d_ol[0].d_buf ) );
	d_plane.d_hRuler = new SpinPoint1DView( d_plane.d_viewer, 
		DimY, 0, ( Qt::darkYellow) );
    d_plane.d_hRulerMdl = new ManualSpinSpace( 1 );
	d_plane.d_hRuler->setModel( d_plane.d_hRulerMdl );
	d_plane.d_hRuler->setLabel( SpinPoint1DView::SysTagAll );
	d_plane.d_viewer->getViews()->replace( RULER1, d_plane.d_hRuler );
	d_plane.d_vRuler = new SpinPoint1DView( d_plane.d_viewer, 
		DimX, 0, ( Qt::darkYellow) );
    d_plane.d_vRulerMdl = new ManualSpinSpace( 1 );
	d_plane.d_vRuler->setModel( d_plane.d_vRulerMdl );
	d_plane.d_vRuler->setLabel( SpinPoint1DView::SysTagAll );
	d_plane.d_viewer->getViews()->replace( RULER2, d_plane.d_vRuler );

	d_plane.d_pp = new PeakSubSpace( new PeakSpaceDummy( s_defDim ), DimX, DimY );
	d_plane.d_peaks = new PeakPlaneView( d_plane.d_viewer, d_plane.d_pp );
	d_plane.d_peaks->setColor( g_clrPeak );
	d_plane.d_viewer->getViews()->replace( PEAKS, d_plane.d_peaks );


	d_plane.d_viewer->getHandlers()->append( new ZoomCtrl( d_plane.d_viewer, true, true ) );
	d_plane.d_viewer->getHandlers()->append( new SelectZoomCtrl( d_plane.d_viewer, true, true ) );
	d_plane.d_viewer->getHandlers()->append( new ScrollCtrl( d_plane.d_viewer ) );
	d_plane.d_viewer->getHandlers()->append( new CursorCtrl( cv, false ) );
	d_plane.d_viewer->getHandlers()->append( 
			new PointSelect1DCtrl( d_plane.d_hRuler, false ) );
	d_plane.d_viewer->getHandlers()->append( 
			new PointSelect1DCtrl( d_plane.d_vRuler, false ) );
	// NOTE: Select muss vor Cursor kommen, da sonst Selection zu sp� passiert.
	d_plane.d_viewer->getHandlers()->append( 
		new PointSelectCtrl( d_plane.d_tuples, false ) );
	d_plane.d_viewer->getHandlers()->append( 
		new PeakSelectCtrl( d_plane.d_peaks, false, false ) ); // Kein Drag-Select
	d_plane.d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_popPlane, false ) );
	d_plane.d_viewer->getHandlers()->append( new FocusCtrl( d_plane.d_viewer ) );
}

void PolyScopeAgent2::initOverlay(int n)
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

void PolyScopeAgent2::setActiveOverlay(int n)
{
	if( n == d_aol )
		return;
	d_plane.d_viewer->redraw();

	d_aol = n;
	updatePlaneLabel();
}

int PolyScopeAgent2::selectLayer()
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

void PolyScopeAgent2::createStrip(Dimension d)
{
	d_strips[ d ].d_viewer = new SpecViewer( 
		new ViewAreaMdl( true, true, false, true ), VIEWCOUNT );
	d_strips[ d ].d_viewer->getViewArea()->addObserver( this );

	d_strips[ d ].d_buf = new SpecBufferMdl( d_strips[ d ].d_viewer->getViewArea(), 0, false );
	d_strips[ d ].d_buf->setFolding( d_folding, false );

	d_strips[ d ].d_view = new ContourView( d_strips[ d ].d_buf, true );	// Immer auto
	d_strips[ d ].d_viewer->getViews()->replace( BACKGROUND, new Lexi::Background() );
	d_strips[ d ].d_viewer->getViews()->replace( CONTOUR, d_strips[ d ].d_view );
	d_strips[ d ].d_view->createLevelsAuto( s_contourFactor, s_contourOption, s_gain );
	d_strips[ d ].d_viewer->getViews()->replace( INTENSITY, 
		new CenterLine( d_strips[ d ].d_viewer ) );

	d_strips[ d ].d_cur = new CursorMdl();
	d_strips[ d ].d_cur->addObserver( this );
	Rotation rot( 3 );
	const Dimension other = (d==DimX)?DimY:DimX;
	rot[ DimX ] = d;
	rot[ DimY ] = DimZ;
	rot[ DimZ ] = other;
	d_strips[ d ].d_mdl = new RangeFilterSpace( d_mdl3D, other );
	d_strips[ d ].d_tuples = new SpinPointView( d_strips[ d ].d_viewer, 
		new RotatedSpace( d_strips[ d ].d_mdl, rot ) );
	d_strips[ d ].d_tuples->setLabel( SpinPointView::SysOrResiTagAll, DimY );
	d_strips[ d ].d_viewer->getViews()->replace( TUPLES, d_strips[ d ].d_tuples );
	CursorView* cv = new CursorView( d_strips[ d ].d_viewer, 
		d_strips[ d ].d_cur, false, true );
	d_strips[ d ].d_viewer->getViews()->replace( CURSORS, cv );
	if( d_folding )
		d_strips[ d ].d_viewer->getViews()->replace( FOLDING, 
			new FoldingView( d_strips[ d ].d_buf ) );

	d_strips[ d ].d_pp = new PeakSubSpace( new PeakSpaceDummy( s_defDim ), d, DimZ );
	d_strips[ d ].d_peaks = new PeakPlaneView( d_strips[ d ].d_viewer, d_strips[ d ].d_pp );
	d_strips[ d ].d_peaks->setColor( g_clrPeak );
	d_strips[ d ].d_viewer->getViews()->replace( PEAKS, d_strips[ d ].d_peaks );

	d_strips[ d ].d_viewer->getHandlers()->append( new ZoomCtrl( d_strips[ d ].d_viewer, false, true ) );
	d_strips[ d ].d_viewer->getHandlers()->append( new SelectZoomCtrl( d_strips[ d ].d_viewer, false, true ) );
	d_strips[ d ].d_viewer->getHandlers()->append( new ScrollCtrl( d_strips[ d ].d_viewer, false, true ) );
	d_strips[ d ].d_viewer->getHandlers()->append( new CursorCtrl( cv, false, false, true ) );
	d_strips[ d ].d_viewer->getHandlers()->append( new 
		PointSelectCtrl( d_strips[ d ].d_tuples, false ) );
	d_strips[ d ].d_viewer->getHandlers()->append( 
		new PeakSelectCtrl( d_strips[ d ].d_peaks, false, false ) ); // Kein Drag-Select
	d_strips[ d ].d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_popStrip, false ) );
	d_strips[ d ].d_viewer->getHandlers()->append( new FocusCtrl( d_strips[ d ].d_viewer ) );
}

void PolyScopeAgent2::createSlice(Dimension view, Dimension spec)
{
	SpecViewer* slice = new SpecViewer( 
		new ViewAreaMdl( view == DimX, view == DimY, view == DimX, view == DimY ) );
	d_slices[ spec ].d_viewer = slice;
	slice->getViewArea()->addObserver( this );

	d_slices[ spec ].d_buf2D = new SpecBufferMdl( slice->getViewArea(), 0, false );
	d_slices[ spec ].d_buf2D->setFolding( d_folding, false );
	slice->getViews()->append( new SliceView( d_slices[ spec ].d_buf2D ) );

	d_slices[ spec ].d_buf3D = new SpecBufferMdl( slice->getViewArea(), 0, false );
	d_slices[ spec ].d_buf3D->setFolding( d_folding, false );
	slice->getViews()->append( new SliceView( d_slices[ spec ].d_buf3D, g_clrSlice3D ) );

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

void PolyScopeAgent2::updateContour( int i, bool redraw )
{
	if( !d_plane.d_ol[0].d_view->isVisi() )
		return;

	if( d_show3DPlane && i == 0 )
	{
		if( d_strips[DimX].d_view->isAuto() )
		{
			d_plane.d_ol[0].d_view->createLevelsAuto( d_strips[DimX].d_view->getFactor(), 
				d_strips[DimX].d_view->getOption(), d_strips[DimX].d_view->getGain() );
		}else
			d_plane.d_ol[0].d_view->createLevelsMin( d_strips[DimX].d_view->getFactor(), 
				(d_spec3D)?d_spec3D->getThreshold():0.0, d_strips[DimX].d_view->getOption() );
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

void PolyScopeAgent2::showIntens(bool on )
{
	if( d_plane.d_intens->isVisi() == on )
		return;
	Lexi::Viewport::pushHourglass();
	d_plane.d_intens->setVisi( on );
	Lexi::Viewport::popCursor();
}

void PolyScopeAgent2::initParams()
{
	d_resol = 1;
	d_lowResol = false;
	d_autoContour = true;
	d_autoCenter = true;
	d_contourFactor = 1.4f;
	d_contourOption = ContourView::Both;
	d_folding = false;
	d_show3DPlane = false;
	d_autoHide = true;
	d_cursorSync = false;
	d_gain = 2.0;
	d_rangeSync = false;
	d_autoHold = false;
	d_syncDepth = true;
	// TODO: diese Werte sollen ab Konfigurations-Record gelesen werden
}

void PolyScopeAgent2::centerToCursor(bool threeD)
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
	}else if( !d_strips.empty() )
	{
		ViewAreaMdl* area = d_strips[0].d_viewer->getViewArea();
		if( !area->getRange( DimY ).contains( d_cursor[ DimZ ] ) )
		{
			area->centerPointDim( DimY, d_cursor[ DimZ ] );
			d_strips[0].d_viewer->damageMe();
		}
	}
}

void PolyScopeAgent2::handle(Root::Message& msg)
{
	if( d_lock )
		return;
	d_lock = true;
	BEGIN_HANDLER();
	MESSAGE( ViewAreaMdl::Update, a, msg )
	{
		Lexi::Viewport::pushHourglass();
		if( a->getOrigin() == d_plane.d_viewer->getViewArea() )
			updatePlane( a );
		else
		{
			Dimension d;
			for( d = 0; d < d_slices.size(); d++ )
				if( d_slices[ d ].d_viewer->getViewArea() == a->getOrigin() )
				{
					updateSlice( d, a );
					break;
				}
			for( d = 0; d < d_strips.size(); d++ )
				if( d_strips[ d ].d_viewer->getViewArea() == a->getOrigin() )
				{
					updateStrip( d, a );
					break;
				}
		}

		Lexi::Viewport::popCursor();
		msg.consume();
	}
	MESSAGE( CursorMdl::Update, a, msg )
	{
		if( a->getOrigin() == d_plane.d_cur )
			updatePlane( a );
		else
		{
			Dimension d;
			for( d = 0; d < d_slices.size(); d++ )
				if( d_slices[ d ].d_cur == a->getOrigin() )
				{
					updateSlice( d, a );
					break;
				}
			for( d = 0; d < d_strips.size(); d++ )
				if( d_strips[ d ].d_cur == a->getOrigin() )
				{
					updateStrip( d, a );
					break;
				}
		}
		msg.consume();
	}
	MESSAGE( GlobalCursor::UpdatePos, a, msg )
	{
		d_lock = false;
		d_cursorSync = false;
		bool threeD = false;
		if( ( a->getDim() == DimY || a->getDim() == DimUndefined ) &&
			d_plane.d_ol[0].d_spec->getColor( DimY ) == a->getTy() )
			d_plane.d_cur->setCursor( Dimension( DimY ), a->getY() );
		if( ( a->getDim() == DimX || a->getDim() == DimUndefined ) &&
			d_plane.d_ol[0].d_spec->getColor( DimX ) == a->getTx() )
			d_plane.d_cur->setCursor( Dimension( DimX ), a->getX() );
		if( d_syncDepth && ( a->getDim() == DimZ ) && d_spec3D &&
			d_spec3D->getColor( DimZ ) == a->getTx() )
		{
			d_cursor[ DimZ ] = a->getX();
			if( !d_strips.empty() )
			{
				d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, a->getX() );
				d_strips[ DimX ].d_cur->setCursor( (Dimension)DimY, a->getX() );
				d_strips[ DimY ].d_cur->setCursor( (Dimension)DimY, a->getX() );
			}
			threeD = true;
		}
		d_cursorSync = true;
		if( !threeD )
		{
			selectCurSystem();
			if( !d_rangeSync )
				centerToCursor( threeD );
		}else
			centerToCursor( threeD );
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
		EXECUTE_ACTION( PolyScopeAgent2, *a );
	}
	MESSAGE( Project::Changed, a, msg )
	{
		msg.consume();
		if( a->d_hint == Project::Width )
		{
			if( d_spec3D )
				d_plane.d_mdl3D->setGhostWidth( d_pro->inferPeakWidth( DimZ, d_spec3D ) );
			if( d_show3DPlane )
				d_plane.d_pp->setRange( DimZ, d_pro->inferPeakWidth( DimZ, d_spec3D ) );
			syncStripsToCur();
		}else if( a->d_hint == Project::WidthFactor )
		{
			syncStripsToCur();
		}
	}
	MESSAGE( SpectrumPeer::Added, a, msg )
	{
        Q_UNUSED(a)
		updateSpecPop2D();
		updateSpecPop3D();
	}
	MESSAGE( SpectrumPeer::Removed, a, msg )
	{
        Q_UNUSED(a)
		updateSpecPop2D();
		updateSpecPop3D();
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

bool PolyScopeAgent2::askToClosePeaklist()
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

bool PolyScopeAgent2::savePeakList()
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

void PolyScopeAgent2::updatePlane(CursorMdl::Update * msg)
{
	// Auf der Plane wurde der Cursor ge�dert
	assert( d_slices.size() >= 2 );
	PpmPoint pos( msg->getX(), msg->getY() );

	if( d_autoCenter && msg->getDim() == DimUndefined )
	{
		Spectrum* spec = d_spec2D;
		if( d_spec3D && d_show3DPlane )
			spec = d_spec3D;
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
	// repr�entierten Slice mit Origin Y (und umgekehrt)
	if( msg->getDim() == DimY || msg->getDim() == DimUndefined )
	{
		d_cursor[ DimY ] = pos[ DimY ];
		d_slices[ DimY ].d_cur->setCursor( (Dimension)DimY, pos[ DimY ] );
		d_slices[ DimX ].d_spec2D->setOrigin( DimY, pos[ DimY ] );
		d_slices[ DimX ].d_viewer->redraw();
		sync3dXySliceToCur( DimX, false );
		if( d_cursorSync )
			GlobalCursor::setCursor( DimY, pos[ DimY ], d_plane.d_ol[0].d_spec->getColor( DimY ) );
	}
	if( msg->getDim() == DimX || msg->getDim() == DimUndefined )
	{
		d_cursor[ DimX ] = pos[ DimX ];
		d_slices[ DimX ].d_cur->setCursor( (Dimension)DimX, pos[ DimX ] );
		d_slices[ DimY ].d_spec2D->setOrigin( DimX, pos[ DimX ] );
		d_slices[ DimY ].d_viewer->redraw();
		sync3dXySliceToCur( DimY, false );
		if( d_cursorSync )
			GlobalCursor::setCursor( DimX, pos[ DimX ], d_plane.d_ol[0].d_spec->getColor( DimX ) );
	}
	sync3dZSliceToCur();
	syncStripsToCur();
	notifyCursor();
}

void PolyScopeAgent2::updateSlice(Dimension dim, CursorMdl::Update *msg)
{
	// Auf einem Slice wurde der Cursor ge�dert
	d_cursor[ dim ] = msg->getX();
	if( dim < 2 )
	{
		// X/Y-Slice gecklickt
		d_plane.d_cur->setCursor( dim, msg->getX() ); // Beide Dims gleich
		syncStripsToCur();
		sync3dZSliceToCur();
		notifyCursor();
		if( d_cursorSync )
			GlobalCursor::setCursor( dim, msg->getX(), 
				d_slices[ dim ].d_spec2D->getColor( DimX ) );
	}else if( d_spec3D && dim == DimZ )
	{
		// Z-Slice geklickt
		// registerPlane();
		sync3dXySliceToCur( DimX, true );
		sync3dXySliceToCur( DimY, true );
		sync2dPlaneSliceToCur();
		d_strips[ DimX ].d_cur->setCursor( (Dimension)DimY, d_cursor[ dim ] );
		d_strips[ DimY ].d_cur->setCursor( (Dimension)DimY, d_cursor[ dim ] );
		notifyCursor( false );
		if( d_cursorSync && d_syncDepth )
			GlobalCursor::setCursor( DimZ, msg->getX(), d_spec3D->getColor( DimZ ) );
	}
}

void PolyScopeAgent2::setCursor( PpmPoint p)
{
	if( p.size() == 0 )
	{
		p.assign( d_spec2D->getDimCount(), 0 );
		for( int i = 0; i < p.size(); i++ )
			p[ i ] = d_spec2D->getScale( i ).getIdxN();
	}
	for( Dimension d = 0; d < p.size(); d++ )
		d_cursor[ d ] = p[ d ];
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
    centerToCursor( false ); // RISK

	sync3dXySliceToCur( DimX, true );
	sync3dXySliceToCur( DimY, true );
	sync2dPlaneSliceToCur();
	if( p.size() == d_cursor.size() && !d_strips.empty() )
	{
		d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
		d_strips[ DimX ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
		d_strips[ DimY ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
	}
	// TODO: Selection
	notifyCursor( true );
}

void PolyScopeAgent2::sync2dPlaneSliceToCur()
{
	if( d_spec3D.isNull() )
		return;
	d_slices[ DimX ].d_spec3D->setOrigin( d_cursor );
	d_slices[ DimY ].d_spec3D->setOrigin( d_cursor );

	d_slices[ DimX ].d_viewer->redraw();
	d_slices[ DimY ].d_viewer->redraw();
	if( d_show3DPlane || d_spec2D->getDimCount() > 2 )
	{
		Lexi::Viewport::pushHourglass();
		d_plane.d_ol[0].d_spec->setOrigin( d_cursor );
		d_plane.d_viewer->redraw();
		Lexi::Viewport::popCursor();
	}
	assert( d_spec3D );
	Lexi::Viewport::pushHourglass();
	PPM w = d_spec3D->getScale( DimZ ).getDelta();
	d_plane.d_mdl3D->setOrigThick( d_cursor[ DimZ ], w );
	if( d_show3DPlane )
		d_plane.d_pp->setOrigThick( DimZ, d_cursor[ DimZ ], w );
	Lexi::Viewport::popCursor();
}

void PolyScopeAgent2::updateSlice(Dimension dim, ViewAreaMdl::Update *msg)
{
	// In Slice wurde Ausschnitt ge�dert
	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;
	if( dim < 2 )
	{
		// X/Y-Slice ver�dert
		d_plane.d_viewer->getViewArea()->setRange( dim, 
			d_slices[ dim ].d_viewer->getViewArea()->getRange( dim ) );
		d_plane.d_viewer->redraw();
		if( d_rangeSync )
			GlobalCursor::setRange( dim, 
				d_slices[ dim ].d_viewer->getViewArea()->getRange( dim ), 
				d_slices[ dim ].d_spec2D->getColor( DimX ) );
	}else if( d_spec3D && dim == DimZ )
	{
		// Z-Slice ver�der
		d_strips[ DimX ].d_viewer->getViewArea()->setRange( DimY, 
			d_slices[ DimZ ].d_viewer->getViewArea()->getRange( DimY ) );
		d_strips[ DimX ].d_viewer->redraw();
		d_strips[ DimY ].d_viewer->getViewArea()->setRange( DimY, 
			d_slices[ DimZ ].d_viewer->getViewArea()->getRange( DimY ) );
		d_strips[ DimY ].d_viewer->redraw();
	}
}

void PolyScopeAgent2::updatePlane(ViewAreaMdl::Update * msg)
{
	// In Plane wurde Ausschnitt ge�dert
	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;
	PpmCube cube;
	cube.assign( d_spec2D->getDimCount(), PpmRange() );
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

void PolyScopeAgent2::updateStrip(Dimension dim, ViewAreaMdl::Update *msg)
{
	// In einem Strip wurde Ausschnitt ge�dert

	if( d_spec3D.isNull() || msg->getType() != ViewAreaMdl::Update::Range )
		return;
	//registerPlane();

	d_slices[ DimZ ].d_viewer->getViewArea()->setRange( DimY, 
			d_strips[ dim ].d_viewer->getViewArea()->getRange( DimY ) );
	d_slices[ DimZ ].d_viewer->redraw();

	Dimension other = !dim; // Hack zum X/Y-Swap
	d_strips[ other ].d_viewer->getViewArea()->setRange( DimY, 
			d_strips[ dim ].d_viewer->getViewArea()->getRange( DimY ) );
	d_strips[ other ].d_viewer->redraw();
}

void PolyScopeAgent2::updateStrip(Dimension dim, CursorMdl::Update *msg)
{
	// In einem Strip wurde Cursor ge�dert

	if( d_spec3D.isNull() )
		return;

	PPM pos = msg->getX();
	if( d_autoCenter )
	{
		SpinPoint tuple = 
			d_strips[ dim ].d_tuples->getHit( d_cursor[ dim ], pos );
		if( !tuple.isZero() )
		{
			pos = tuple[ DimY ]->getShift( d_spec3D ); 
			msg->override( pos, pos ); 
		}else if( d_pl && d_pl->getDimCount() > 2 )
		{
			Root::Index peak = d_strips[ dim ].d_peaks->getHit( d_cursor[ dim ], pos );
			if( peak )
			{
				PeakPos p;
				d_strips[ dim ].d_peaks->getModel()->getPos( peak, p, d_spec3D );
				msg->override( p[ DimY ], p[ DimY ] ); 
				pos = p[ DimY ];
				// das geht, da View die Message erst nach Agent erh�t.
			}
		}
	}

	// registerPlane();
	d_cursor[ DimZ ] = pos;
	sync3dXySliceToCur( DimX, true );
	sync3dXySliceToCur( DimY, true );
	sync2dPlaneSliceToCur();
	d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
	Dimension other = !dim; // Hack zum X/Y-Swap
	d_strips[ other ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
	if( d_show3DPlane )
		updateContour( 0, true );
	notifyCursor( false );
	if( d_cursorSync && d_syncDepth )
		GlobalCursor::setCursor( DimZ, d_cursor[ DimZ ], d_spec3D->getColor( DimZ ) );
}

void PolyScopeAgent2::updatePlaneLabel()
{
	Spectrum* spec = d_plane.d_ol[d_aol].d_spec;
	if( !d_spec3D.isNull() && d_show3DPlane )
		spec = d_spec3D;
	QString str;
	if( d_plane.d_ol.size() > 1 )
	{
		if( spec == 0 )
			str.sprintf( " %d <empty>", d_aol );
		else
			str.sprintf( " %d %s  %s / %s", d_aol, spec->getName(), spec->getDimName( DimX ),
				spec->getDimName( DimY ) );
	}else
		str.sprintf( " %s  %s / %s", spec->getName(), spec->getDimName( DimX ),
			spec->getDimName( DimY ) );
	d_plane.d_viewer->getViews()->replace( LABEL1,
		new Lexi::Label( str, nil, 
		(d_show3DPlane)?g_clrSlice3D:d_plane.d_tuples->getColor(), 
		Lexi::AlignLeft, Lexi::AlignTop ) );
}

void PolyScopeAgent2::setSpec3D(Spectrum * spec)
{
	if( d_spec3D == spec )
		return;

	// Ist egal: assert( spec == 0 || d_specList.count( spec ) > 0 ); 
	// Nur vorher evaluierte Spektren zul�sig.

	Lexi::Viewport::pushHourglass();
	Spectrum* old = d_spec3D;
	d_spec3D = spec;
	if( !d_spec3D.isNull() )
	{
		d_slices[ DimZ ].d_spec3D = new SpecProjector( d_spec3D, DimZ );
		d_slices[ DimZ ].d_spec3D->setOrigin( d_cursor );

		d_strips[ DimX ].d_spec = new SpecProjector( d_spec3D, DimX, DimZ );
		d_strips[ DimY ].d_spec = new SpecProjector( d_spec3D, DimY, DimZ );

		d_slices[ DimZ ].d_buf3D->setSpectrum( d_slices[ DimZ ].d_spec3D, true );
		d_slices[ DimZ ].d_spec2D = 0;
		d_slices[ DimZ ].d_buf2D->setSpectrum( 0 );

		if( old == 0 || old->getColor( DimZ ) != spec->getColor( DimZ ) ||
			!old->getScale( DimZ ).getRange().intersects( spec->getScale( DimZ ).getRange() ) )
		{
			// Nur FitWindow, wenn andere Farbe oder Bereich nicht berlappend
			PpmRange r = d_spec3D->getScale( DimZ ).getRange();	
			r.invert();
			d_strips[ DimX ].d_viewer->getViewArea()->setRange( DimY, r );
			d_strips[ DimY ].d_viewer->getViewArea()->setRange( DimY, r );
			d_slices[ DimZ ].d_viewer->getViewArea()->setRange( DimY, r );
		}

		d_mdl3D->setSpec( 0 );
		d_src3D->setSpec( d_spec3D );
		d_mdl3D->setSpec( d_spec3D );
		d_plane.d_mdl3D->setGhostWidth( d_pro->inferPeakWidth( DimZ, d_spec3D ) );
		syncStripsToCur();

		d_strips[ DimX ].d_viewer->getViews()->replace( LABEL1,
			new Lexi::Label( d_spec3D->getName(), nil, g_clrSlice3D, 
			Lexi::AlignLeft, Lexi::AlignTop ) );
		updateStripLabelPop();
	}else
	{
		d_slices[ DimZ ].d_spec3D = 0;
		d_strips[ DimX ].d_spec = 0;
		d_strips[ DimY ].d_spec = 0;
		d_slices[ DimZ ].d_buf3D->setSpectrum( 0 );
		if( d_spec2D->getDimCount() > 2 )
		{
			d_slices[ DimZ ].d_spec2D = new SpecProjector( d_spec2D, DimZ );
			d_slices[ DimZ ].d_buf2D->setSpectrum( d_slices[ DimZ ].d_spec2D );
			d_slices[ DimZ ].d_buf2D->fitToArea();
		}
		d_strips[ DimX ].d_viewer->getViews()->replace( LABEL1, 0 );
		d_mdl3D->setSpec( 0 );
		d_src3D->setSpecType( 0 );
		if( d_popLabel )
		{
			d_popLabel->purge();
			Gui::Menu::item( d_popLabel, this, "?", 0, false );
			Gui::Menu::item( d_popLabel, this, "?-1", 0, false );
		}
	}

	d_slices[ DimZ ].d_viewer->redraw();
	d_strips[ DimX ].d_buf->setSpectrum( d_strips[ DimX ].d_spec );
	d_strips[ DimY ].d_buf->setSpectrum( d_strips[ DimY ].d_spec );
	syncStripWidth();

	if( d_show3DPlane )
	{
		update3dPlane();
		updatePlaneLabel();
	}

	sync3dXySliceToCur( DimX, true );
	sync3dXySliceToCur( DimY, true );
	selectCurSystem( true );
	Lexi::Viewport::popCursor();
	SpecChanged msg( true );
	traverseUp( msg );
}

void PolyScopeAgent2::updateStripLabelPop()
{
	if( d_popLabel )
	{
		d_popLabel->purge();
		Gui::Menu::item( d_popLabel, this, "?", LabelSpin3D, true )->addParam( Root::Any() );
		Gui::Menu::item( d_popLabel, this, "?-1", LabelSpin3D, true )
			->addParam( Root::Any( new SpinLabelHolder( "?-1" ) ) );
		const SpinLabelSet& sls = d_spec3D->getType()->getLabels( 
			d_spec3D->mapToType( DimZ ) );
		SpinLabelSet::const_iterator p1;
		typedef std::map<QByteArray ,SpinLabel> Sort;
		Sort sort;
		Sort::const_iterator q1;
		for( p1 = sls.begin(); p1 != sls.end(); ++p1 )
		{
			sort[ (*p1).data() ] = (*p1);
		}
		for( q1 = sort.begin(); q1 != sort.end(); ++q1 )
		{
			Gui::Menu::item( d_popLabel, this, (*q1).first.data(), 
				LabelSpin3D, true )->addParam( (*q1).second.data() );
		}
	}
}

void PolyScopeAgent2::stepSpec2D(bool next)
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

void PolyScopeAgent2::stepSpec3D(bool next)
{
	if( d_sort3D.size() < 2 )
		return;
	Sort::const_iterator p = d_sort3D.end();
	if( d_spec3D )
		p = d_sort3D.find( d_spec3D->getName() );
	if( p == d_sort3D.end() )
	{
		if( next )
			p = d_sort3D.begin();
		else
			--p;
	}else
	{
		if( next )
		{
			++p;
			if( p == d_sort3D.end() )
				p = d_sort3D.begin();
		}else if( p == d_sort3D.begin() )
		{
			p = d_sort3D.end();
			--p;
		}else
			--p;
	}
	assert( p != d_sort3D.end() );
	setSpec3D( (*p).second );
}

void PolyScopeAgent2::setSpec2D(Spectrum * spec )
{
	if( d_spec2D == spec )
		return;
	Lexi::Viewport::pushHourglass();
	d_spec2D = spec;

	d_plane.d_ol[0].d_spec = new SpecProjector( d_spec2D, DimX, DimY );
	d_plane.d_ol[0].d_buf->setSpectrum( d_plane.d_ol[0].d_spec );

	d_mdl2D->setSpec( 0 );
	d_src2D->setSpec( d_spec2D );
	d_mdl2D->setSpec( d_spec2D );
	if( d_spec2D->getDimCount() > 2 )
	{
		d_plane.d_tuples->setModel( d_plane.d_mdl3D );
		d_plane.d_ol[0].d_spec->setOrigin( d_cursor );
	}else
		d_plane.d_tuples->setModel( d_mdl2D );

	d_plane.d_vRulerMdl->setSpec( d_plane.d_ol[0].d_spec );
	d_plane.d_hRulerMdl->setSpec( d_plane.d_ol[0].d_spec );

	d_slices[ DimX ].d_spec2D = new SpecProjector( d_spec2D, DimX );
	d_slices[ DimX ].d_buf2D->setSpectrum( d_slices[ DimX ].d_spec2D );
	d_slices[ DimX ].d_buf2D->setFolding( d_folding );
	d_slices[ DimY ].d_spec2D = new SpecProjector( d_spec2D, DimY );
	d_slices[ DimY ].d_buf2D->setSpectrum( d_slices[ DimY ].d_spec2D );
	d_slices[ DimY ].d_buf2D->setFolding( d_folding );
	d_plane.d_peaks->setSpec( d_spec2D );

	if( d_spec2D->getDimCount() > 2 )
	{
		d_slices[ DimX ].d_spec2D->setOrigin( d_cursor );
		d_slices[ DimY ].d_spec2D->setOrigin( d_cursor );
	}else
	{
		d_slices[ DimX ].d_spec2D->setOrigin( DimY, d_cursor[ DimY ] );
		d_slices[ DimY ].d_spec2D->setOrigin( DimX, d_cursor[ DimX ] );
	}

	updateContour( 0, true );
	updatePlaneLabel();
	d_plane.d_viewer->redraw();
	Lexi::Viewport::popCursor();
	SpecChanged msg( false );
	traverseUp( msg );
}

void PolyScopeAgent2::setPeakList(PeakList * pl)
{
	if( pl )
	{
		d_pl = new PeakListPeer( pl );
		d_plane.d_pp->setPeakSpace( d_pl );
		if( !d_strips.empty() )
		{
			d_strips[DimX].d_pp->setPeakSpace( d_pl );
			d_strips[DimY].d_pp->setPeakSpace( d_pl );
		}
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
		if( !d_strips.empty() )
		{
			d_strips[DimX].d_pp->setPeakSpace( new PeakSpaceDummy( s_defDim ) );
			d_strips[DimY].d_pp->setPeakSpace( new PeakSpaceDummy( s_defDim ) );
		}
		d_plane.d_viewer->getViews()->replace( LABEL3, 0 );
	}
}

void PolyScopeAgent2::extendSystem(Dimension source, Dimension target )
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
		ProposeSpinDlg dlg( MW, d_pro, d_spec2D->getColor( source ), d_cursor[ source ], 
			d_spec2D,	"Select Reference Spin" );
		dlg.setAnchor( source, ref );
		if( !dlg.exec() || dlg.getSpin() == 0 )
			return;
		ref = dlg.getSpin();
	}
	pickSpin( target, ref, ref->getSystem() );
}

void PolyScopeAgent2::pickSpin(Dimension d, Spin *other, SpinSystem *owner)
{
	SpinLabel l = d_spec2D->getKeyLabel( d );

	// 26.6.05: immer anzeigen if( !d_src2D->showNulls() )
	{
		SpinLabelSet ly = d_spec2D->getType()->getLabels( d_spec2D->mapToType( d ) );
			// Ich lasse das vorerst so, da nicht sicher ist, ob Inference Keys enth�t.

		NmrExperiment* e = d_pro->getRepository()->getTypes()->inferExperiment2(
					d_spec2D->getType(), owner, d_spec2D );
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

Spin* PolyScopeAgent2::getSelectedSpin()
{
	Dimension ref = DimX;
	if(	d_strips[ DimX ].d_viewer->hasFocus() )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() )
		ref = DimY;
	if( d_strips[ ref ].d_tuples->getSel().size() != 1 )
		return 0;
	return ( *d_strips[ ref ].d_tuples->getSel().begin() )[ DimY ];
}

void PolyScopeAgent2::gotoTuple(SpinSystem * sys, Spin * spin, Spin * link, bool twoD )
{
	SpinPoint tuple;
	Dimension dim;
	SpinSpace::Result tuples;
	if( !twoD && d_spec3D )
	{
		if( link )
			d_mdl3D->find( tuples, spin, link );
		else if( spin )
			d_mdl3D->find( tuples, spin );
		else
			d_mdl3D->find( tuples, sys );
		dim = 3;
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
	if( tuples.hasOne() || d_synchro )
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
			( dim == 3 )?d_spec3D:d_spec2D ) );
	bool ac = d_autoCenter;
	d_autoCenter = false;
	d_plane.d_cur->setCursor( orig[ DimX ], orig[ DimY ] );
	d_plane.d_tuples->select( tuple );
	if( dim == 3 )
	{
		d_cursor[ DimZ ] = orig[ DimZ ];
		sync3dXySliceToCur( DimX, true );
		sync3dXySliceToCur( DimY, true );
		sync2dPlaneSliceToCur();
		d_strips[ DimX ].d_cur->setCursor( (Dimension)DimY, orig[ DimZ ] );
		d_strips[ DimY ].d_cur->setCursor( (Dimension)DimY, orig[ DimZ ] );
	}
	// TODO: select in Plane and Strips
	selectCurSystem();
	centerToCursor();
	if( dim == 3 )
	{
		ViewAreaMdl* area = d_strips[ DimX ].d_viewer->getViewArea();
		if( !area->getRange( DimY ).contains( d_cursor[ DimZ ] ) )
		{
			area->centerPoint( area->getRange( DimX ).getCenter(), d_cursor[ DimZ ] );
			d_strips[ DimX ].d_viewer->damageMe();
		}
	}
	notifyCursor( dim == 2 );
	d_autoCenter = ac;
}

Spin* PolyScopeAgent2::getSel(bool hori) const
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

void PolyScopeAgent2::notifyCursor(bool plane)
{
	QString str;
    QTextStream ts( &str, QIODevice::WriteOnly );

	ts.setf( QTextStream::fixed );
	ts.precision( 3 );

	ts <<  "Cursor:  ";

	Spectrum* spec = 0;
	if( plane )
		spec = d_spec2D;
	else
		spec = d_spec3D;

	for( Dimension d = 0; d < spec->getDimCount(); d++ )
	{
		ts << char( 'x' + d ) << ": ";
		// ts << spec->getColor( d ).getLabel() << "=";
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
		if( plane )
		{
			PpmPoint p;
			for( Dimension d = 0; d < d_spec2D->getDimCount(); d++ )
				p.push_back( d_cursor[ d ] );
			val = d_spec2D->getAt( p, d_folding, d_folding );
		}else 
			val = d_spec3D->getAt( d_cursor, d_folding, d_folding );
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
	else if( d_strips[ DimX ].d_viewer->hasFocus() )
		tv = d_strips[ DimX ].d_tuples;
	else if( d_strips[ DimY ].d_viewer->hasFocus() )
		tv = d_strips[ DimY ].d_tuples;
	if( tv && tv->formatSelection( tmp, SpinPointView::PairAll, 3 ) )
	{
		str += ",  ";
		str += tmp.data();
	}
	if( plane && d_plane.d_peaks->formatSelection( tmp ) )
	{
		str += ",  ";
		str += tmp.data();
	}
	Lexi::ShowStatusMessage msg( str );
	traverseUp( msg );
}

void PolyScopeAgent2::sync3dZSliceToCur()
{
	if( d_spec3D.isNull() )
		return;

	d_slices[ DimZ ].d_spec3D->setOrigin( d_cursor );
	d_slices[ DimZ ].d_viewer->redraw();
}

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

void PolyScopeAgent2::syncStripWidth()
{
	if( d_spec3D.isNull() )
		return;
	PpmRange r = d_spec2D->getScale( DimX ).getRange();	
	PPM w = d_pro->inferStripWidth( DimX, d_spec3D );
	_allocate( r, d_cursor[ DimX ], w );
	d_strips[ DimX ].d_viewer->getViewArea()->setRange( DimX, r );
	w = d_pro->inferPeakWidth( DimX, d_spec3D );
    d_strips[ DimY ].d_mdl->setGhostWidth( w );
	d_strips[ DimY ].d_pp->setRange( DimX, w );

	r = d_spec2D->getScale( DimY ).getRange();
	w = d_pro->inferStripWidth( DimY, d_spec3D );
	_allocate( r, d_cursor[ DimY ], w );
	d_strips[ DimY ].d_viewer->getViewArea()->setRange( DimX, r );
	w = d_pro->inferPeakWidth( DimY, d_spec3D );
    d_strips[ DimX ].d_mdl->setGhostWidth( w );
	d_strips[ DimX ].d_pp->setRange( DimY, w );

	d_strips[ DimX ].d_viewer->redraw();
	d_strips[ DimY ].d_viewer->redraw();
}

void PolyScopeAgent2::syncStripsToCur()
{
	if( d_spec3D.isNull() )
		return;
	d_lock = true;

	d_strips[ DimX ].d_spec->setOrigin( d_cursor );
	d_strips[ DimY ].d_spec->setOrigin( d_cursor );

    d_strips[ DimX ].d_mdl->setOrigThick( d_cursor[ DimY ],
		d_spec3D->getScale( DimY ).getDelta(), true );	// RISK
    d_strips[ DimY ].d_mdl->setOrigThick( d_cursor[ DimX ],
		d_spec3D->getScale( DimX ).getDelta(), true );

	d_strips[ DimX ].d_pp->setOrigThick( DimY, d_cursor[ DimY ], 
		d_spec3D->getScale( DimY ).getDelta() );
	d_strips[ DimY ].d_pp->setOrigThick( DimX, d_cursor[ DimX ], 
		d_spec3D->getScale( DimX ).getDelta() );

	syncStripWidth();
	selectCurSystem();

	d_lock = false;
}

void PolyScopeAgent2::sync3dXySliceToCur( Dimension dim, bool show )
{
	assert( dim == DimX || dim == DimY );

	Dimension d = dim; // TODO * 2;

	if( ( !d_autoHide || show ) && !d_spec3D.isNull() )
	{
		// RISK: wieso? if( d_slices[ dim ].d_spec3D.isNull() )
			d_slices[ dim ].d_spec3D = new SpecProjector( d_spec3D, d );
		d_slices[ dim ].d_spec3D->setOrigin( d_cursor );
	}else
	{
		d_slices[ dim ].d_spec3D = 0;
	}
	if( d_spec2D->getDimCount() > 2 )
		d_slices[ dim ].d_spec2D->setOrigin( d_cursor );

	d_slices[ dim ].d_buf3D->setSpectrum( d_slices[ dim ].d_spec3D );
	d_slices[ dim ].d_viewer->redraw();
}

void PolyScopeAgent2::updateContour2( Dimension d, bool ac )
{
	if( ac )
	{
		d_strips[d].d_view->createLevelsAuto();
	}else
		d_strips[d].d_view->createLevelsMin( (d_spec3D)?d_spec3D->getThreshold():0.0 );
	d_strips[d].d_viewer->damageMe();
}

void PolyScopeAgent2::updateRef()
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
	d_plane.d_viewer->redraw(); // TODO: eingrenzen
}

void PolyScopeAgent2::selectCurSystem( bool force )
{
	if( d_spec3D && ( d_plane.d_tuples->getSel().size() == 1 ||
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

		// H, N
		// H, N, C
		QString str;
		if( d_synchro && d_cur[0]->getSystem() )
			str.sprintf( " Sys. %d", d_cur[0]->getSystem()->getId() );
		else
		{
			char buf[32];
			SpinPointView::formatLabel( buf, sizeof(buf), d_cur, 
				SpinPointView::PairIdLabelSysOrResi, DimY );
			str = buf;
		}
		d_strips[ DimY ].d_viewer->getViews()->replace( LABEL1,
			new Lexi::Label( str, nil, g_clrSlice3D, 
			Lexi::AlignLeft, Lexi::AlignTop ) );

			str.sprintf( " %s/%s", 
				d_spec3D->getDimName( DimX ), d_spec3D->getDimName( DimZ ) );

		d_strips[ DimX ].d_viewer->getViews()->replace( LABEL2,
			new Lexi::Label( str, nil, g_clrSlice3D, 
			Lexi::AlignLeft, Lexi::AlignBottom ) );
		str.sprintf( " %s/%s", 
			d_spec3D->getDimName( DimY ), d_spec3D->getDimName( DimZ ) );
		d_strips[ DimY ].d_viewer->getViews()->replace( LABEL2,
			new Lexi::Label( str, nil, g_clrSlice3D, 
			Lexi::AlignLeft, Lexi::AlignBottom ) );

		d_strips[ DimX ].d_mdl->setSys( d_cur[ DimX ]->getSystem() );
		d_strips[ DimY ].d_mdl->setSys( d_cur[ DimY ]->getSystem() );
	}else
	{
		d_cur.zero();
		if( !d_strips.empty() )
		{
			d_strips[ DimY ].d_viewer->getViews()->replace( LABEL1, 0 );
			d_strips[ DimX ].d_viewer->getViews()->replace( LABEL2, 0 );
			d_strips[ DimY ].d_viewer->getViews()->replace( LABEL2, 0 );
			d_strips[ DimX ].d_mdl->setSys( 0 );
			d_strips[ DimY ].d_mdl->setSys( 0 );
		}
	}
}

void PolyScopeAgent2::handleSetResolution(Action & a)
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

void PolyScopeAgent2::handleShowLowRes(Action & a)
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

void PolyScopeAgent2::handleForward(Action & a)
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

void PolyScopeAgent2::handleBackward(Action & a)
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

void PolyScopeAgent2::handleFitWindow(Action & a)
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

void PolyScopeAgent2::handleShowFolded(Action & a)
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
		d_slices[ d ].d_buf3D->setFolding( d_folding );
		d_slices[ d ].d_viewer->redraw();
	}
	for( d = 0; d < d_strips.size(); d++ )
	{
		d_strips[ d ].d_buf->setFolding( d_folding );
		if( d_folding )
			d_strips[ d ].d_viewer->getViews()->replace( FOLDING, 
				new FoldingView( d_strips[ d ].d_buf ) );
		else
			d_strips[ d ].d_viewer->getViews()->replace( FOLDING, 0 );
		d_strips[ d ].d_viewer->redraw();
	}
	Viewport::popCursor();
}

void PolyScopeAgent2::handleSpecCalibrate(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	Spectrum* spec = d_spec2D;
	if( d_spec3D && d_show3DPlane )
		spec = d_spec3D;

	PpmPoint p( 0, 0 );
	for( Dimension d = 0; d < 2; d++ )
		p[ d ] = tuple[ d ]->getShift( spec ) - d_cursor[ d ];

	Viewport::pushHourglass();
	Root::Ref<SpecCalibrateCmd> cmd = new SpecCalibrateCmd( spec, p );
	cmd->handle( this );
	Viewport::popCursor();
}

void PolyScopeAgent2::handleAutoCenter(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_autoCenter );

	d_autoCenter = !d_autoCenter;
}

void PolyScopeAgent2::handleShowContour(Action & a)
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

void PolyScopeAgent2::handleShowIntensity(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_plane.d_intens->isVisi() );

	showIntens( !d_plane.d_intens->isVisi() );
}

void PolyScopeAgent2::handleAutoContour(Action & a)
{
    ACTION_CHECKED_IF( a, ( d_show3DPlane && d_aol == 0 ) || d_plane.d_ol[d_aol].d_spec,
        ( d_show3DPlane && d_strips[ DimX ].d_view->isAuto() ) ||
        ( !d_show3DPlane && d_plane.d_ol[d_aol].d_view->isAuto() ) );
	
	if( d_show3DPlane && d_aol == 0 )
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

void PolyScopeAgent2::handleContourParams(Action & a)
{
    ACTION_ENABLED_IF( a, ( d_show3DPlane && d_aol == 0 ) || d_plane.d_ol[d_aol].d_spec );

	if( d_show3DPlane && d_aol == 0 )
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

void PolyScopeAgent2::handlePickSystem(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	PpmPoint p( d_cursor[ DimX ], d_cursor[ DimY ] );
	bool ok = true;
	if( !d_show3DPlane && d_spec2D->getDimCount() == 2 && !d_synchro )
	{
		// Kann kein System picken in einem 3D ohne dritte Dim auch zu picken.
		/* 26.6.05: ab nun immer Dialog
		if( d_src2D->showNulls() )
		{
			Root::Ref<CreateSystemPairCmd> c1 = 
				new CreateSystemPairCmd( d_pro->getSpins(),	p, d_spec2D );
			ok = c1->handle( this ); // Darstellung von ? m�lich.
		}else
		*/
		{
			Dlg::LP lp;
			// Darstellung von ? nicht m�lich. Picke mit Label.
			lp.d_x = d_spec2D->getKeyLabel( DimX );
			lp.d_y = d_spec2D->getKeyLabel( DimY );

			if( !Dlg::getLabelsSysType( MW, lp, d_pro->getRepository(), d_spec2D->getType(), 
				d_spec2D->mapToType( DimX ), d_spec2D->mapToType( DimY ) ) )
				return;
			Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick System" );
			Root::Ref<CreateSystemPairCmd> c1 = 
				new CreateSystemPairCmd( d_pro->getSpins(),	p, d_spec2D, lp.d_sys );
			c1->execute();
			cmd->add( c1 );
			if( !c1->getSpin( DimX )->getLabel().equals( lp.d_x ) )
				cmd->add( new LabelSpinCmd( d_pro->getSpins(), c1->getSpin( DimX ), lp.d_x ) ); 
			if( !c1->getSpin( DimY )->getLabel().equals( lp.d_y ) )
				cmd->add( new LabelSpinCmd( d_pro->getSpins(), c1->getSpin( DimY ), lp.d_y ) ); 
			ok = cmd->handle( this );
			if( lp.d_x.isNull() || lp.d_y.isNull() )
				d_src2D->showNulls( true );
		}
	}else if( d_synchro )
	{
		Root::Ref<CreateSystemPairCmd> c1 = 
			new CreateSystemPairCmd( d_pro->getSpins(),	p, d_spec2D );
		ok = c1->handle( this );
	}else // if( d_show3DPlane )
	{
		Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick System" );
		Root::Ref<CreateSystemPairCmd> c1 = 
			new CreateSystemPairCmd( d_pro->getSpins(),	p, d_spec2D );
		c1->execute();
		cmd->add( c1 );
		Root::Ref<PickSystemSpinCmd> c2 = new PickSystemSpinCmd( d_pro->getSpins(), 
			c1->getSystem(), d_spec3D->getColor( DimZ ), d_cursor[ DimZ ], 0 ); 
		cmd->add( c2 );
		c2->execute();
		ok = cmd->handle( this );
	}
	if( ok )
	{
		d_plane.d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimY ] );
		if( d_autoHold && !d_plane.d_tuples->getSel().empty() )
			d_ref = *d_plane.d_tuples->getSel().begin();
		updateRef();
		selectCurSystem();
		if( !d_strips.empty() )
		{
			d_strips[ DimX ].d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimZ ] );
			d_strips[ DimY ].d_tuples->selectPeak( d_cursor[ DimY ], d_cursor[ DimZ ] );
		}
	}
}

void PolyScopeAgent2::handlePickHori(Action & a)
{
	ACTION_ENABLED_IF( a, !d_show3DPlane );
	extendSystem( DimY, DimX );
}

void PolyScopeAgent2::handlePickVerti(Action & a)
{
	ACTION_ENABLED_IF( a, !d_show3DPlane );
	extendSystem( DimX, DimY );
}

void PolyScopeAgent2::handleMovePeak(Action & a)
{
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

void PolyScopeAgent2::handleMovePeakAlias(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spin System" );
	Spectrum* spec = (d_show3DPlane && d_spec3D)?d_spec3D:d_spec2D;
	for( Dimension d = 0; d < 2; d++ )
	{
		if( d_cursor[ d ] != tuple[ d ]->getShift( spec ) )
			cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ d ], 
				d_cursor[ d ], spec ) );
	}
	cmd->handle( this );
	selectCurSystem( true );
}

void PolyScopeAgent2::handleLabelPeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	Spectrum* spec = d_spec2D;
	if( d_show3DPlane && d_spec3D )
		spec = d_spec3D;
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
	if( !Dlg::getLabels( MW, 
		tuple[ DimX ]->getId(), tuple[ DimY ]->getId(), x, y, lx, ly ) )
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

void PolyScopeAgent2::handleHidePeak(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinLink* link = tuple[ DimX ]->findLink( tuple[ DimY ] );
	ACTION_ENABLED_IF( a, link );
	Root::Ref<HideSpinLinkCmd> cmd = new HideSpinLinkCmd( d_pro->getSpins(), 
		link, d_spec2D );
	cmd->handle( this );
	// TODO: Plural
}

void PolyScopeAgent2::handleDeletePeak(Action & a)
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

void PolyScopeAgent2::handleDeleteSpinX(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimX ] );
	cmd->handle( this );
}

void PolyScopeAgent2::handleDeleteSpinY(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimY ] );
	cmd->handle( this );
}

void PolyScopeAgent2::handleShowAllPeaks(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_mdl2D->showAll() );
	d_mdl2D->showAll( !d_mdl2D->showAll() );
	d_mdl3D->showAll( d_mdl2D->showAll() );
}

void PolyScopeAgent2::handleShowAlignment(Action & a)
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

void PolyScopeAgent2::handleAssign(Action & a)
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

void PolyScopeAgent2::handleUnassign(Action & a)
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

void PolyScopeAgent2::handleSetSystemType(Action & a)
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

void PolyScopeAgent2::handleViewLabels(Action & a)
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

void PolyScopeAgent2::handleSelectSpec2D(Action & a)
{
	ACTION_CHECKED_IF( a, !d_show3DPlane, d_spec2D == a.getParam( 0 ).getObject() );

	setSpec2D( dynamic_cast<Spectrum*>( a.getParam( 0 ).getObject() ) );
}

void PolyScopeAgent2::handleLinkSystems(Action & a)
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

void PolyScopeAgent2::handleUnlinkPred(Action & a)
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

void PolyScopeAgent2::handleUnlinkSucc(Action & a)
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

void PolyScopeAgent2::handleUnhidePeak(Action & a)
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

void PolyScopeAgent2::handleHoldReference(Action & a)
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

void PolyScopeAgent2::handleCreateReport(Action & a)
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

void PolyScopeAgent2::handleAddRulerVerti(Action & a)
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

void PolyScopeAgent2::handleAddRulerHori(Action & a)
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

void PolyScopeAgent2::handleRemoveRulers(Action & a)
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

void PolyScopeAgent2::handleRemoveAllRulers(Action & a)
{
	ACTION_ENABLED_IF( a, !d_plane.d_hRulerMdl->isEmpty() ||
		!d_plane.d_vRulerMdl->isEmpty() );
	d_plane.d_hRulerMdl->removeAll();
	d_plane.d_vRulerMdl->removeAll();
}

void PolyScopeAgent2::handleAutoRuler(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_autoRuler );

	d_autoRuler = !d_autoRuler;
}

void PolyScopeAgent2::handleProposeHori(Action & a)
{
	if( d_ref.isZero() && d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = d_ref;
	if( d_ref[ DimX ] == 0 )
		tuple = *d_plane.d_tuples->getSel().begin();

	Spin* ref = tuple[ DimY ];
	ACTION_ENABLED_IF( a, !d_show3DPlane && ref->getSystem() && d_spec2D->hasNoesy() );

	ProposeSpinDlg dlg( MW, d_pro, d_spec2D->getColor( DimX ), d_cursor[ DimX ], 
		d_spec2D, "Select Horizontal Partner" );
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

void PolyScopeAgent2::handleProposeVerti(Action & a)
{
	if( d_ref.isZero() && d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = d_ref;
	if( d_ref[ DimX ] == 0 )
		tuple = *d_plane.d_tuples->getSel().begin();

	Spin* ref = tuple[ DimX ];
	ACTION_ENABLED_IF( a, !d_show3DPlane && ref->getSystem() && d_spec2D->hasNoesy()  );

	ProposeSpinDlg dlg( MW, d_pro, d_spec2D->getColor( DimY ), d_cursor[ DimY ], 
		d_spec2D, "Select Vertical Partner" );
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

void PolyScopeAgent2::handleProposePeak(Action & a)
{
    ACTION_ENABLED_IF( a, ( d_show3DPlane && d_spec3D && d_spec3D->hasNoesy() ) ||
        ( !d_show3DPlane && d_spec2D->hasNoesy() ) );

	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Propose Peak" );
	Dlg::SpinTuple res;
	if( d_show3DPlane && d_spec3D )
	{
		Dlg::SpinRanking l1 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_spec3D->getColor( DimX ), d_cursor[ DimX ], d_spec3D );
		Dlg::SpinRanking l2 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_spec3D->getColor( DimY ), d_cursor[ DimY ], d_spec3D );
		Dlg::SpinRanking l3 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_spec3D->getColor( DimZ ), d_cursor[ DimZ ], d_spec3D );
		res = Dlg::selectSpinTriple( MW, 
			l1, l2, l3, "Select Matching Spins" );
		if( res.empty() )
			return;

		if( d_spec3D->isNoesy( DimX, DimY ) && res[DimX]->findLink( res[DimY] ) == 0 )
			cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimX], res[DimY] ) ); 

		if( d_spec3D->isNoesy( DimX, DimZ ) && res[DimX]->findLink( res[DimZ] ) == 0 )
			cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimX], res[DimZ] ) ); 

		if( d_spec3D->isNoesy( DimY, DimZ ) && res[DimY]->findLink( res[DimZ] ) == 0 )
			cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimY], res[DimZ] ) ); 
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

void PolyScopeAgent2::handleSelectSpec3D(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_spec3D == a.getParam( 0 ).getObject() );

	setSpec3D( dynamic_cast<Spectrum*>( a.getParam( 0 ).getObject() ) );
	selectCurSystem();
}

void PolyScopeAgent2::handlePickSpin3D(Action & a)
{
	if( !d_src3D->showNulls() )
	{
		handlePickLabel3D( a );
		return;
	}

	Dimension ref = DimX;	// RISK
	if(	d_strips[ DimX ].d_viewer->hasFocus() )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() )
		ref = DimY;
	ACTION_ENABLED_IF( a, ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );

	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick Spin" );
	// Pick generisches Spektrum
	if( d_cur[ ref ]->getSystem() )
	{
		Root::Ref<PickSystemSpinCmd> c2 = new PickSystemSpinCmd( d_pro->getSpins(), 
			d_cur[ ref ]->getSystem(), d_spec3D->getColor( DimZ ), d_cursor[ DimZ ], 0 ); 
		cmd->add( c2 );
		c2->execute();
	}else
	{
		Root::Ref<PickSpinCmd> c2 = new PickSpinCmd( d_pro->getSpins(), 
			d_spec3D->getColor( DimZ ), d_cursor[ DimZ ], 0 ); 
		cmd->add( c2 );
		c2->execute();
	}
	
	cmd->handle( this );
	d_strips[ DimX ].d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimZ ] );
	d_strips[ DimY ].d_tuples->selectPeak( d_cursor[ DimY ], d_cursor[ DimZ ] );
}

void PolyScopeAgent2::handleMoveSpin3D(Action & a)
{
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		d_strips[ DimX ].d_tuples->getSel().size() == 1 )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		d_strips[ DimY ].d_tuples->getSel().size() == 1 )
		ref = DimY;

	ACTION_ENABLED_IF( a, ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );

	SpinPoint tuple;
	if( ref == DimX )
		tuple = *d_strips[ DimX ].d_tuples->getSel().begin();
	else
		tuple = *d_strips[ DimY ].d_tuples->getSel().begin();
	Root::Ref<MoveSpinCmd> cmd =
		new MoveSpinCmd( d_pro->getSpins(), tuple[ DimY ], d_cursor[ DimZ ], 0 );
		// Move generisches Spektrum
	cmd->handle( this );
}

void PolyScopeAgent2::handleMoveSpinAlias3D(Action & a)
{
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		d_strips[ DimX ].d_tuples->getSel().size() == 1 )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		d_strips[ DimY ].d_tuples->getSel().size() == 1 )
		ref = DimY;

	ACTION_ENABLED_IF( a, ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );

	SpinPoint tuple;
	if( ref == DimX )
		tuple = *d_strips[ DimX ].d_tuples->getSel().begin();
	else
		tuple = *d_strips[ DimY ].d_tuples->getSel().begin();
	Root::Ref<MoveSpinCmd> cmd =
		new MoveSpinCmd( d_pro->getSpins(), tuple[ DimY ], d_cursor[ DimZ ], d_spec3D );
	cmd->handle( this );
}

void PolyScopeAgent2::handleDeleteSpins3D(Action & a)
{
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		!d_strips[ DimX ].d_tuples->getSel().empty() )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		!d_strips[ DimY ].d_tuples->getSel().empty() )
		ref = DimY;

	ACTION_ENABLED_IF( a, ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );

	SpinPointView::Selection sel;
	if( ref == DimX )
		sel = d_strips[ DimX ].d_tuples->getSel();
	else
		sel = d_strips[ DimY ].d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Spins" );
	std::set<Spin*> test;
	for( p = sel.begin(); p != sel.end(); ++p )
	{
		if( test.count( (*p)[ DimY ] ) == 0 )
		{
			cmd->add( new DeleteSpinCmd( d_pro->getSpins(), (*p)[ DimY ] ) );
			test.insert( (*p)[ DimY ] );
		}
	}
	cmd->handle( this );
}

void PolyScopeAgent2::handleLabelSpin3D(Action & a)
{
	if( d_spec3D.isNull() || d_cur.isZero() )
		return;
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		d_strips[ DimX ].d_tuples->getSel().size() == 1 )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		d_strips[ DimY ].d_tuples->getSel().size() == 1 )
		ref = DimY;

	ACTION_ENABLED_IF( a, ref != DimUndefined );

	SpinPoint tuple;
	if( ref == DimX )
		tuple = *d_strips[ DimX ].d_tuples->getSel().begin();
	else
		tuple = *d_strips[ DimY ].d_tuples->getSel().begin();

	SpinLabel y = tuple[ DimY ]->getLabel();
	if( a.getParam( 0 ).isNull() || !SpinLabel::parse( a.getParam( 0 ).getCStr(), y ) )
	{
		SpinLabelSet ly = d_spec3D->getType()->getLabels( d_spec3D->mapToType( DimZ ) );
		// ly soll vorerst alle statischen Labels auch sehen. Wenn man das wegl�st,
		// verschwinden zur Zeit noch CB aus der Liste von HNCACB von Freds Template.
		// Zu grosse Auswirkungen auf bestehende Projekte.
		NmrExperiment* e = d_pro->getRepository()->getTypes()->
			inferExperiment2( d_spec3D->getType(), tuple[ DimY ]->getSystem(), d_spec3D );
		if( e )
		{
			// e->getColumn( d_spec3D->mapToType( DimZ ), ly );
            PathTable::Path filter;
			filter[ d_spec3D->mapToType( DimX ) ] = d_cur[ DimX ]->getLabel();
			filter[ d_spec3D->mapToType( DimY ) ] = d_cur[ DimY ]->getLabel();
			e->getColumn( filter, d_spec3D->mapToType( DimZ ), ly );
		}
		if( tuple[ DimY ]->getSystem() )
			ly = tuple[ DimY ]->getSystem()->getAcceptables( ly );
		if( !Dlg::getLabel( MW, y, ly ) )
			return;
	}

	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_pro->getSpins(), tuple[ DimY ], y );
	cmd->handle( this );
	if( y.isNull() )
		d_src3D->showNulls( true );
}

void PolyScopeAgent2::handleForceLabelSpin3D(Action & a)
{
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		d_strips[ DimX ].d_tuples->getSel().size() == 1 )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		d_strips[ DimY ].d_tuples->getSel().size() == 1 )
		ref = DimY;

	ACTION_ENABLED_IF( a, ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );

	SpinPoint tuple;
	if( ref == DimX )
		tuple = *d_strips[ DimX ].d_tuples->getSel().begin();
	else
		tuple = *d_strips[ DimY ].d_tuples->getSel().begin();

	bool ok	= FALSE;
	QString res;
	if( a.getParam( 0 ).isNull() )
	{
		res.sprintf( "Please enter a valid label (%s):", SpinLabel::s_syntax );
		res = QInputDialog::getText( "Force Spin Label", res, QLineEdit::Normal, 
			tuple[ DimY ]->getLabel().data(), &ok, MW );
		if( !ok )
			return;
	}else
		res = a.getParam( 0 ).getCStr();

	SpinLabel l;
	if( !SpinLabel::parse( res, l ) )
	{
		Root::ReportToUser::alert( this, "Force Spin Label", "Invalid spin label syntax!" );
		return;
	}

	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_pro->getSpins(), tuple[ DimY ], l );
	cmd->handle( this );
	if( l.isNull() )
		d_src3D->showNulls( true );
}

void PolyScopeAgent2::handleSetWidth(Action & a)
{
	ACTION_ENABLED_IF( a, !d_spec3D.isNull() &&
		( d_strips[ DimX ].d_viewer->hasFocus() ||
		  d_strips[ DimY ].d_viewer->hasFocus() ) );

	Dimension dim;
	if( d_strips[ DimX ].d_viewer->hasFocus() )
	{
		dim = DimX;
	}else
	{
		dim = DimY;
	}
	bool ok	= FALSE;
	QString res;
	res.sprintf( "%0.3f", d_pro->inferPeakWidth( dim, d_spec3D ) );
	res	= QInputDialog::getText( "Set Peak Width", 
		"Please	enter atom positive PPM value:",QLineEdit::Normal,  
		res, &ok, MW );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok || w < 0.0 )
	{
		QMessageBox::critical( MW, "Set Peak Width",
				"Invalid peak width!", "&Cancel" );
		return;
	}

	d_pro->setPeakWidth( dim, w, d_spec3D );
}

void PolyScopeAgent2::handleFitWindow3D(Action & a)
{
	ACTION_ENABLED_IF( a, !d_spec3D.isNull() ); 
	if( d_spec3D.isNull() )
		return;	// QT-BUG
	PpmRange r = d_spec3D->getScale( DimZ ).getRange();	
	r.invert();
	d_strips[ DimX ].d_viewer->getViewArea()->setRange( DimY, r );
	d_strips[ DimY ].d_viewer->getViewArea()->setRange( DimY, r );
	d_slices[ DimZ ].d_viewer->getViewArea()->setRange( DimY, r );	
	d_strips[ DimX ].d_viewer->redraw();
	d_strips[ DimY ].d_viewer->redraw();
}

void PolyScopeAgent2::handleAutoContour2(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_strips[ DimX ].d_view->isAuto() );
	
	bool ac = !d_strips[ DimX ].d_view->isAuto();
	updateContour2( DimX, ac );
	updateContour2( DimY, ac );
	if( d_show3DPlane )
		updateContour( 0, true );
}

void PolyScopeAgent2::handleContourParams2(Action & a)
{
	ACTION_ENABLED_IF( a, d_spec3D );

	Dlg::ContourParams p;
	p.d_factor = d_strips[ DimX ].d_view->getFactor();
	p.d_threshold =	d_spec3D->getThreshold();
	p.d_option = d_strips[ DimX ].d_view->getOption();
	if( Dlg::setParams( MW, p ) )
	{
		d_spec3D->setThreshold( p.d_threshold );
		d_strips[ DimX ].d_view->setOption( p.d_option );
		d_strips[ DimX ].d_view->setFactor( p.d_factor );
		d_strips[ DimY ].d_view->setOption( p.d_option );
		d_strips[ DimY ].d_view->setFactor( p.d_factor );
		updateContour2( DimX, false );
		updateContour2( DimY, false );
		if( d_show3DPlane )
		{
			showIntens( false );
			updateContour( 0, true );
		}
	}
}

void PolyScopeAgent2::handleShowWithOff(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src3D->showOffs() );
	
	d_src3D->showOffs( !d_src3D->showOffs() );
}

void PolyScopeAgent2::update3dPlane()
{
	if( d_show3DPlane )
	{
		d_plane.d_ol[0].d_spec = new SpecProjector( d_spec3D, DimX, DimY );
		d_plane.d_ol[0].d_spec->setOrigin( d_cursor );
		d_plane.d_ol[0].d_buf->setSpectrum( d_plane.d_ol[0].d_spec );
		// TODO if( !d_synchro )
			d_plane.d_tuples->setModel( d_plane.d_mdl3D );
		d_plane.d_pp->setOrigThick( DimZ, d_cursor[ DimZ ], d_spec3D->getScale( DimZ ).getDelta() );
		d_plane.d_pp->setRange( DimZ, d_plane.d_mdl3D->getGhostWidth() );
		d_plane.d_peaks->setSpec( d_spec3D );
	}else
	{
		d_plane.d_ol[0].d_spec = new SpecProjector( d_spec2D, DimX, DimY );
		d_plane.d_ol[0].d_buf->setSpectrum( d_plane.d_ol[0].d_spec );
		if( d_spec2D->getDimCount() > 2 )
		{
			d_plane.d_ol[0].d_spec->setOrigin( d_cursor );
			d_slices[ DimX ].d_spec2D->setOrigin( d_cursor );
			d_slices[ DimY ].d_spec2D->setOrigin( d_cursor );
		}
		// TODO if( !d_synchro )
			d_plane.d_tuples->setModel( d_mdl2D );
		d_plane.d_pp->setOrigThick( DimZ, d_cursor[ DimZ ], 0 );
		d_plane.d_pp->setRange( -1, 0 );
		d_plane.d_peaks->setSpec( d_spec2D );
	}
	d_plane.d_vRulerMdl->setSpec( d_plane.d_ol[0].d_spec );
	d_plane.d_hRulerMdl->setSpec( d_plane.d_ol[0].d_spec );
	updateContour( 0, true );
	updatePlaneLabel();
	d_plane.d_viewer->redraw();
}

void PolyScopeAgent2::handleShow3dPlane(Action & a)
{
	ACTION_CHECKED_IF( a, d_spec3D, d_show3DPlane );
	d_show3DPlane = !d_show3DPlane;
	if( d_show3DPlane )
	{
		d_contourFactor = d_plane.d_ol[0].d_view->getFactor();
		d_contourOption = d_plane.d_ol[0].d_view->getOption();
		d_gain = d_plane.d_ol[0].d_view->getGain();
		d_autoContour = d_plane.d_ol[0].d_view->isAuto();
	}else
	{
		if( d_autoContour )
		{
			d_plane.d_ol[0].d_view->createLevelsAuto( d_contourFactor, 
				d_contourOption, d_gain );
		}else
			d_plane.d_ol[0].d_view->createLevelsMin( d_contourFactor, 
				d_spec2D->getThreshold(), d_contourOption );
	}
	update3dPlane();
	SpecChanged msg( true );
	traverseUp( msg );
}

void PolyScopeAgent2::handleAutoHide(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_autoHide );

	d_autoHide = !d_autoHide;
	if( d_spec3D )
	{
		sync3dXySliceToCur( DimX, !d_autoHide );
		sync3dXySliceToCur( DimY, !d_autoHide );
	}
	d_slices[ DimX ].d_viewer->redraw();
	d_slices[ DimY ].d_viewer->redraw();
}

void PolyScopeAgent2::handleStripCalibrate(Action & a)
{
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		d_strips[ DimX ].d_tuples->getSel().size() == 1 )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		d_strips[ DimY ].d_tuples->getSel().size() == 1 )
		ref = DimY;

	ACTION_ENABLED_IF( a, ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );


	SpinPoint tuple;
	if( ref == DimX )
		tuple = *d_strips[ DimX ].d_tuples->getSel().begin();
	else
		tuple = *d_strips[ DimY ].d_tuples->getSel().begin();

	PpmPoint p( 0, 0, 0 );
	p[ DimZ ] = tuple[ DimY ]->getShift( d_spec3D ) - d_cursor[ DimZ ];

	Viewport::pushHourglass();
	Root::Ref<SpecCalibrateCmd> cmd = new SpecCalibrateCmd( d_spec3D, p );
	cmd->handle( this );
	Viewport::popCursor();
}

void PolyScopeAgent2::handleProposeSpin(Action & a)
{
	ACTION_ENABLED_IF( a, !d_spec3D.isNull() && !d_cur.isZero() && 
		d_spec3D->isNoesy( DimZ ) );

	Spin* orig = 0;
	if( d_spec3D->isNoesy( DimX, DimZ ) )
		orig = d_cur[ DimX ];
	else
		orig = d_cur[ DimY ];

	ProposeSpinDlg dlg( MW, d_pro, d_spec3D->getColor( DimZ ), d_cursor[ DimZ ], 
		d_spec3D, "Select Vertical Partner" );
	dlg.setAnchor( DimX, d_cur[ DimX ] );
	dlg.setAnchor( DimZ, d_cur[ DimY ] );
	if( !dlg.exec() || dlg.getSpin() == 0 )
		return;

	if( orig->findLink( dlg.getSpin() ) == 0 ) // Ref == target zulssig wegen Diagonaler
	{
		Root::Ref<LinkSpinCmd> c1 = new LinkSpinCmd( d_pro->getSpins(), orig, dlg.getSpin() ); 
		if( c1->handle( this ) )
		{
			d_strips[ DimX ].d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimZ ] );
			d_strips[ DimY ].d_tuples->selectPeak( d_cursor[ DimY ], d_cursor[ DimZ ] );
		}
	}else
		Root::ReportToUser::alert( this, "Propose Vertical Extension", 
			"The selected spins are already linked!" );
}

void PolyScopeAgent2::handleEditAttsSpinH(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimX ] );
}

void PolyScopeAgent2::handleEditAttsSpinV(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimY ] );
}

void PolyScopeAgent2::handleEditAttsLink(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
	ACTION_ENABLED_IF( a, l );

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keyLink ), l );
}

void PolyScopeAgent2::handleEditAttsSysH(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinSystem* s = tuple[ DimX ]->getSystem();
	ACTION_ENABLED_IF( a, s );

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ), s );
}

void PolyScopeAgent2::handleEditAttsSysV(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinSystem* s = tuple[ DimY ]->getSystem();
	ACTION_ENABLED_IF( a, s );

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ), s );
}

void PolyScopeAgent2::handleEditAttsSpin3D(Action & a)
{
	Spin* spin = getSelectedSpin();
	ACTION_ENABLED_IF( a, !d_spec3D.isNull() && !d_cur.isZero() && spin);

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keySpin ), spin );
}

void PolyScopeAgent2::handleCursorSync(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_cursorSync );
	
	d_cursorSync = !d_cursorSync;
	if( d_cursorSync )
		GlobalCursor::addObserver( this );	// TODO: preset Cursor
	else if( !d_rangeSync )
		GlobalCursor::removeObserver( this );
}

void PolyScopeAgent2::handleGotoSystem(Action & a)
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

void PolyScopeAgent2::handleNextSpec3D(Action & a)
{
	ACTION_ENABLED_IF( a, d_sort3D.size() > 1 ); 
	stepSpec3D( true );
}

void PolyScopeAgent2::handlePrevSpec3D(Action & a)
{
	ACTION_ENABLED_IF( a, d_sort3D.size() > 1 ); 
	stepSpec3D( false );
}

void PolyScopeAgent2::handleNextSpec2D(Action & a)
{
	ACTION_ENABLED_IF( a, d_sort2D.size() > 1 ); 
	stepSpec2D( true );
}

void PolyScopeAgent2::handlePrevSpec2D(Action & a)
{
	ACTION_ENABLED_IF( a, d_sort2D.size() > 1 ); 
	stepSpec2D( false );
}

void PolyScopeAgent2::handleForwardPlane(Action & a)
{
	if( d_spec3D.isNull() )
		return;
	ACTION_ENABLED_IF( a, !d_spec3D.isNull() ); 

	d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, 
		d_cursor[ DimZ ] + d_spec3D->getScale( DimZ ).getDelta() );  
}

void PolyScopeAgent2::handleBackwardPlane(Action & a)
{
	if( d_spec3D.isNull() )
		return;
	ACTION_ENABLED_IF( a, !d_spec3D.isNull() ); 

	d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, 
		d_cursor[ DimZ ] - d_spec3D->getScale( DimZ ).getDelta() );  
}

void PolyScopeAgent2::handleShowWithOff2(Action & a)
{
	ACTION_CHECKED_IF( a, !d_show3DPlane, d_src2D->showOffs() );
	
	d_src2D->showOffs( !d_src2D->showOffs() );
	d_plane.d_viewer->redraw();
}

void PolyScopeAgent2::handleShowLinks(Action & a)
{
	ACTION_CHECKED_IF( a, !d_show3DPlane, d_src2D->showLinks() );
	
	d_src2D->showLinks( !d_src2D->showLinks() );
	d_plane.d_viewer->redraw();
}

void PolyScopeAgent2::handleShowLinks2(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src3D->showLinks() );
	
	d_src3D->showLinks( !d_src3D->showLinks() );
	d_plane.d_viewer->redraw();
}

void PolyScopeAgent2::handleDeleteLinks(Action & a)
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

void PolyScopeAgent2::handleLabelVerti(Action & a)
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

void PolyScopeAgent2::handleLabelHori(Action & a)
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

void PolyScopeAgent2::handleSetCandidates(Action & a)
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

void PolyScopeAgent2::handleShowInfered(Action & a)
{
	ACTION_CHECKED_IF( a, !d_show3DPlane, d_src2D->showInferred() );
	
	d_src2D->showInferred( !d_src2D->showInferred() );
	d_plane.d_viewer->redraw();
}

void PolyScopeAgent2::handleShowInfered2(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src3D->showInferred() );
	
	d_src3D->showInferred( !d_src3D->showInferred() );
	d_plane.d_viewer->redraw();
}

void PolyScopeAgent2::handleShowUnlabeled(Action & a)
{
	ACTION_CHECKED_IF( a, !d_show3DPlane, d_src2D->showNulls() );
	
	d_src2D->showNulls( !d_src2D->showNulls() );
	d_plane.d_viewer->redraw();
}

void PolyScopeAgent2::handleShowUnlabeled2(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src3D->showNulls() );
	
	d_src3D->showNulls( !d_src3D->showNulls() );
	d_plane.d_viewer->redraw();
}

void PolyScopeAgent2::handleShowUnknown(Action & a)
{
	ACTION_CHECKED_IF( a, !d_show3DPlane, d_src2D->showUnknown() );
	
	d_src2D->showUnknown( !d_src2D->showUnknown() );
	d_plane.d_viewer->redraw();
}

void PolyScopeAgent2::handleShowUnknown2(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src3D->showUnknown() );
	
	d_src3D->showUnknown( !d_src3D->showUnknown() );
	d_plane.d_viewer->redraw();
}

void PolyScopeAgent2::handleCreateLinks(Action & a)
{
	ACTION_CHECKED_IF( a, true, false );
}

void PolyScopeAgent2::handleForceCross(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src2D->doPathsim() );
	
	d_src2D->doPathsim( !d_src2D->doPathsim() );
	d_src3D->doPathsim( d_src2D->doPathsim() );
	d_plane.d_viewer->redraw();
}

void PolyScopeAgent2::handleDeleteLinks3D(Action & a)
{
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		!d_strips[ DimX ].d_tuples->getSel().empty() )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		!d_strips[ DimY ].d_tuples->getSel().empty() )
		ref = DimY;

	ACTION_ENABLED_IF( a, ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );

	SpinPointView::Selection sel;
	if( ref == DimX )
		sel = d_strips[ DimX ].d_tuples->getSel();
	else
		sel = d_strips[ DimY ].d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Links" );
	SpinLink* l;
	std::set<SpinLink*> test;
	for( p = sel.begin(); p != sel.end(); ++p )
	{
		// TODO statt (*p)[ DimX ] ev. d_cur[ DimX ] verwenden? Identisch?
		l = (*p)[ DimX ]->findLink( (*p)[ DimY ] );
		if( l && test.count( l ) == 0 )
		{
			cmd->add( new UnlinkSpinCmd( d_pro->getSpins(), (*p)[ DimX ], (*p)[ DimY ] ) );
			test.insert( l );
		}
	}
	cmd->handle( this );
}

void PolyScopeAgent2::handleViewLabels3D(Action & a)
{
	if( a.getParamCount() == 0 )
		return;

	SpinPointView::Label q = (SpinPointView::Label) a.getParam( 0 ).getShort();
	if( q < SpinPointView::None || q >= SpinPointView::End )
		return;

	ACTION_CHECKED_IF( a, true,
		d_strips[ DimX ].d_tuples->getLabel() == q );
	
	for( Dimension d = 0; d < d_strips.size(); d++ )
	{
		d_strips[ d ].d_tuples->setLabel( q, DimY );
		d_strips[ d ].d_viewer->redraw();
	}
}

void PolyScopeAgent2::handleAutoGain(Action & a)
{
	ACTION_ENABLED_IF( a, !a.getParam( 0 ).isNull() && !d_show3DPlane );

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

void PolyScopeAgent2::handleAutoGain3D(Action & a)
{
	ACTION_ENABLED_IF( a, !a.getParam( 0 ).isNull() );
	float g = a.getParam( 0 ).getFloat();
	if( g <= 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Auto Gain", "Invalid Gain Value" );
		return;
	}
	d_strips[ DimX ].d_view->setGain( g );
	d_strips[ DimY ].d_view->setGain( g );
	updateContour2( DimX, true );
	updateContour2( DimY, true );
}

void PolyScopeAgent2::handleShowGhosts(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_plane.d_mdl3D->showGhosts() );

	d_plane.d_mdl3D->showGhosts( !d_plane.d_mdl3D->showGhosts() );
	d_strips[ DimX ].d_mdl->showGhosts( d_plane.d_mdl3D->showGhosts() );
	d_strips[ DimY ].d_mdl->showGhosts( d_plane.d_mdl3D->showGhosts() );
}

void PolyScopeAgent2::handleAutoHold(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_autoHold );
	d_autoHold = !d_autoHold;
}

void PolyScopeAgent2::handlePickLabel3D(Action & a)
{
	Dimension ref = DimX;
	if(	d_strips[ DimX ].d_viewer->hasFocus() )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() )
		ref = DimY;
	ACTION_ENABLED_IF( a, ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() &&
		d_cur[ ref ]->getSystem() );

	SpinLabelSet ly = d_spec3D->getType()->getLabels( d_spec3D->mapToType( DimZ ) );
		// ly soll vorerst alle statischen Labels auch sehen. Wenn man das wegl�st,
		// verschwinden zur Zeit noch CB aus der Liste von HNCACB von Freds Template.
		// Zu grosse Auswirkungen auf bestehende Projekte.
	NmrExperiment* e = d_pro->getRepository()->getTypes()->
		inferExperiment2( d_spec3D->getType(), d_cur[ ref ]->getSystem(), d_spec3D );
	if( e )
	{
		// e->getColumn( d_spec3D->mapToType( DimZ ), ly );
        PathTable::Path filter;
		filter[ d_spec3D->mapToType( DimX ) ] = d_cur[ DimX ]->getLabel();
		filter[ d_spec3D->mapToType( DimY ) ] = d_cur[ DimY ]->getLabel();
		e->getColumn( filter, d_spec3D->mapToType( DimZ ), ly );
	}
	if( d_cur[ ref ]->getSystem() )
		ly = d_cur[ ref ]->getSystem()->getAcceptables( ly );
	SpinLabel y;
	if( a.getParamCount() == 0 || a.getParam( 0 ).isNull() || 
		!SpinLabel::parse( a.getParam( 0 ).getCStr(), y ) )
		if( !Dlg::getLabel( MW, y, ly ) )
			return;
	if( !d_cur[ ref ]->getSystem()->isAcceptable( y ) )
	{
		Root::ReportToUser::alert( this, "Pick Label", "Label is not acceptable" );
		return;
	}

	Root::Ref<PickSystemSpinLabelCmd> cmd = new PickSystemSpinLabelCmd( d_pro->getSpins(), 
		d_cur[ ref ]->getSystem(), d_spec3D->getColor( DimZ ), d_cursor[ DimZ ], y, 0 ); 
	cmd->handle( this );

	if( y.isNull() )
		d_src3D->showNulls( true );

	d_strips[ DimX ].d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimZ ] );
	d_strips[ DimY ].d_tuples->selectPeak( d_cursor[ DimY ], d_cursor[ DimZ ] );
}

void PolyScopeAgent2::handleSetDepth(Action & a)
{
	ACTION_ENABLED_IF( a, d_spec3D );

	float g;
	if( a.getParam( 0 ).isNull() )
	{
		bool ok	= FALSE;
		QString res;
		res.sprintf( "%f", d_pro->inferPeakWidth( DimZ, d_spec3D ) );
		res	= QInputDialog::getText( "Set Peak Depth", 
			"Please	enter a positive ppm value:", QLineEdit::Normal, res, &ok, 
			MW );
		if( !ok )
			return;
		g = res.toFloat( &ok );
	}else
		g = a.getParam( 0 ).getFloat();
	if( g < 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Peak Depth", "Invalid PPM Value" );
		return;
	}
	d_pro->setPeakWidth( DimZ, g, d_spec3D );
}

void PolyScopeAgent2::handleGhostLabels(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_plane.d_tuples->ghostLabel() );

	d_plane.d_tuples->ghostLabel( !d_plane.d_tuples->ghostLabel() );
	d_strips[ DimX ].d_tuples->ghostLabel( d_plane.d_tuples->ghostLabel() );
	d_strips[ DimY ].d_tuples->ghostLabel( d_plane.d_tuples->ghostLabel() );
}

void PolyScopeAgent2::handleHidePeak2(Action & a)
{
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		d_strips[ DimX ].d_tuples->getSel().size() == 1 )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		d_strips[ DimY ].d_tuples->getSel().size() == 1 )
		ref = DimY;

	ACTION_ENABLED_IF( a, ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );

	SpinPoint tuple;
	if( ref == DimX )
		tuple = *d_strips[ DimX ].d_tuples->getSel().begin();
	else
		tuple = *d_strips[ DimY ].d_tuples->getSel().begin();
	SpinLink* link = tuple[ DimX ]->findLink( tuple[ DimY ] );
	ACTION_ENABLED_IF( a, link );
	Root::Ref<HideSpinLinkCmd> cmd = new HideSpinLinkCmd( d_pro->getSpins(), 
		link, d_spec3D );
	cmd->handle( this );
	// TODO: Plural
}

void PolyScopeAgent2::handleGotoPeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_spec3D ); 

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

void PolyScopeAgent2::handleRangeSync(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_rangeSync );
	
	d_rangeSync = !d_rangeSync;
	if( d_rangeSync )
		GlobalCursor::addObserver( this );	// TODO: preset Cursor
	else if( !d_cursorSync )
		GlobalCursor::removeObserver( this );
}

void PolyScopeAgent2::handleEditAttsSys3D(Action & a)
{
	Spin* spin = getSelectedSpin();
	ACTION_ENABLED_IF( a, !d_spec3D.isNull() && !d_cur.isZero() && spin && spin->getSystem() );

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ), 
		spin->getSystem() );
}

void PolyScopeAgent2::handleEditAttsLink3D(Action & a)
{
	Spin* spin = getSelectedSpin();
	SpinLink* l = 0;
	if( d_cur[ DimX ] )
        l = d_cur[ DimX ]->findLink( spin );
	ACTION_ENABLED_IF( a, !d_spec3D.isNull() && !d_cur.isZero() && l );

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keyLink ), l );
}

void PolyScopeAgent2::handleOverlayCount(Action & a)
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

void PolyScopeAgent2::handleActiveOverlay(Action & a)
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

void PolyScopeAgent2::handleSetPosColor(Action & a)
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

void PolyScopeAgent2::handleSetNegColor(Action & a)
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

void PolyScopeAgent2::handleOverlaySpec(Action & a)
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

void PolyScopeAgent2::handleCntFactor(Action & a)
{
	ACTION_ENABLED_IF( a, !d_show3DPlane && !a.getParam( 0 ).isNull() );

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

void PolyScopeAgent2::handleCntThreshold(Action & a)
{
	ACTION_ENABLED_IF( a, !d_show3DPlane && !a.getParam( 0 ).isNull() );

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

void PolyScopeAgent2::handleCntOption(Action & a)
{
	ACTION_ENABLED_IF( a, !d_show3DPlane && !a.getParam( 0 ).isNull() );

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

void PolyScopeAgent2::handleAddLayer(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	initOverlay( d_plane.d_ol.size() + 1 );
	setActiveOverlay( d_plane.d_ol.size() - 1 );
	handleOverlaySpec( a );
}

void PolyScopeAgent2::handleComposeLayers(Action & a)
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

void PolyScopeAgent2::handleUseLinkColors(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_plane.d_tuples->getColors() );
	if( d_plane.d_tuples->getColors() )
		d_plane.d_tuples->setColors( 0 );
	else
		d_plane.d_tuples->setColors( d_pro->getRepository()->getColors() );
}

void PolyScopeAgent2::handleUseLinkColors3D(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_strips[DimX].d_tuples->getColors() );

	if( d_strips[DimX].d_tuples->getColors() )
	{	
		d_strips[DimX].d_tuples->setColors( 0 );
		d_strips[DimY].d_tuples->setColors( 0 );
	}else
	{
		d_strips[DimX].d_tuples->setColors( d_pro->getRepository()->getColors() );
		d_strips[DimY].d_tuples->setColors( d_pro->getRepository()->getColors() );
	}
}

void PolyScopeAgent2::handleSetLinkParams(Action & a)
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
	ACTION_ENABLED_IF( a, l );
	const SpinLink::Alias& al = l->getAlias( d_spec2D );
	Dlg::LinkParams2 par;
	par.d_rating = al.d_rating;
	par.d_code = al.d_code;
	par.d_visible = al.d_visible;
	if( Dlg::getLinkParams2( MW, par ) )
		d_pro->getSpins()->setAlias( l, d_spec2D, 
		par.d_rating, par.d_code, par.d_visible );
}

void PolyScopeAgent2::handleSetLinkParams3D(Action & a)
{
	Spin* spin = getSelectedSpin();
	SpinLink* l = 0;
	if( d_cur[ DimX ] )
        l = d_cur[ DimX ]->findLink( spin );
	ACTION_ENABLED_IF( a, !d_spec3D.isNull() && !d_cur.isZero() && l );

	const SpinLink::Alias& al = l->getAlias( d_spec3D );
	Dlg::LinkParams2 par;
	par.d_rating = al.d_rating;
	par.d_code = al.d_code;
	par.d_visible = al.d_visible;
	if( Dlg::getLinkParams2( MW, par ) )
		d_pro->getSpins()->setAlias( l, d_spec3D, 
		par.d_rating, par.d_code, par.d_visible );
}

void PolyScopeAgent2::handleGotoPoint(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	PpmPoint orig = d_cursor;
	GotoDlg dlg( MW );
	Spectrum* spec = d_spec2D;
	if( d_spec3D )
		spec = d_spec3D;
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

void PolyScopeAgent2::handleNewPeakList(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	if( !askToClosePeaklist() )
		return;

	Spectrum* spec = d_spec2D;
	if( d_spec3D && d_show3DPlane )
		spec = d_spec3D;
	setPeakList( new PeakList( spec ) );
}

void PolyScopeAgent2::handleOpenPeakList(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	PeakList* pl = Dlg::selectPeakList( MW, d_pro );
	if( pl == 0 )
		return;
	setPeakList( pl );
}

void PolyScopeAgent2::handleSavePeakList(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl && d_pl->getId() == 0 );

	savePeakList();
}

void PolyScopeAgent2::handleMapPeakList(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl );

	RotateDlg dlg( MW, "Peaklist", "Spectrum" );
	Rotation rot( d_pl->getDimCount() );
	Spectrum* spec = d_spec2D;
	if( d_spec3D && d_show3DPlane )
		spec = d_spec3D;
	for( Dimension d = 0; d < d_pl->getDimCount(); d++ )
	{
		dlg.addDimension( d_pl->getAtomType( d ).getIsoLabel(), 
		( d < spec->getDimCount() )?spec->getColor( d ).getIsoLabel():"" );
		rot[ d ] = d;
	}

	if( dlg.rotate( rot ) )
		d_pl->setRotation( rot );
}

void PolyScopeAgent2::handlePickPlPeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl );

	assert( d_pl );
	PeakSpace::PeakData pd;
	PeakModel::Params pp;
	pd.d_pos[ DimX ] = d_cursor[ DimX ];
	pd.d_pos[ DimY ] = d_cursor[ DimY ];
	pd.d_pos[ DimZ ] = d_cursor[ DimZ ];
	Spectrum* spec = d_spec2D;
	if( d_spec3D && d_show3DPlane )
		spec = d_spec3D;
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

void PolyScopeAgent2::handleMovePlPeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl && d_plane.d_peaks->getSel().size() == 1 );

	Root::Index peak = *d_plane.d_peaks->getSel().begin();
	PeakPos pos;
	PeakModel::Params pp;
	pos[ DimX ] = d_cursor[ DimX ];
	pos[ DimY ] = d_cursor[ DimY ];
	pos[ DimZ ] = d_cursor[ DimZ ];
	Spectrum* spec = d_spec2D;
	if( d_spec3D && d_show3DPlane )
		spec = d_spec3D;
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

void PolyScopeAgent2::handleMovePlAlias(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl && d_plane.d_peaks->getSel().size() == 1 );

	Root::Index peak = *d_plane.d_peaks->getSel().begin();
	PeakPos pos;
	PeakModel::Params pp;
	pos[ DimX ] = d_cursor[ DimX ];
	pos[ DimY ] = d_cursor[ DimY ];
	pos[ DimZ ] = d_cursor[ DimZ ];
	Spectrum* spec = d_spec2D;
	if( d_spec3D && d_show3DPlane )
		spec = d_spec3D;
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

void PolyScopeAgent2::handleLabelPlPeak(Action & a)
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

void PolyScopeAgent2::handleDeletePlPeaks(Action & a)
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

void PolyScopeAgent2::handleEditPlPeakAtts(Action & a)
{
	ACTION_ENABLED_IF( a, d_pl && d_plane.d_peaks->getSel().size() == 1 );

	Root::Index id = ( *d_plane.d_peaks->getSel().begin() );
	Peak* peak = d_pl->getPeakList()->getPeak( id );
	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keyPeak ), peak );
}

void PolyScopeAgent2::handleSetPlColor(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	QColor clr = QColorDialog::getColor( d_plane.d_peaks->getColor(), 
		MW );
	if( clr.isValid() )
	{
		d_plane.d_peaks->setColor( ( clr ) );
		d_plane.d_viewer->redraw();
		for( int i = 0; i < d_strips.size(); i++ )
		{
			d_strips[i].d_peaks->setColor( d_plane.d_peaks->getColor() );
			d_strips[i].d_viewer->redraw();
		}
	}
}

void PolyScopeAgent2::handleDeleteAliasPeak(Action & a)
{
	ACTION_ENABLED_IF( a, !d_plane.d_tuples->getSel().empty() );

	SpinPointView::Selection sel = d_plane.d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
    d_pro->getSpins();
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Spins" );
	std::set<Spin*> test;
	for( p = sel.begin(); p != sel.end(); ++p )
	{
		if( test.count( (*p)[ DimX ] ) == 0 )
		{
			cmd->add( new MoveSpinCmd( d_pro->getSpins(), (*p)[ DimX ], 
			(*p)[ DimX ]->getShift(), // auf Home schieben l�cht Alias
			d_spec2D ) );
			test.insert( (*p)[ DimX ] );
		}
		if( test.count( (*p)[ DimY ] ) == 0 )
		{
			cmd->add( new MoveSpinCmd( d_pro->getSpins(), (*p)[ DimY ], 
			(*p)[ DimY ]->getShift(), // auf Home schieben l�cht Alias
			d_spec2D ) );
			test.insert( (*p)[ DimY ] );
		}
	}
	cmd->handle( this );
}

void PolyScopeAgent2::handleFitWindowX(Action & a)
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

void PolyScopeAgent2::handleFitWindowY(Action & a)
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

void PolyScopeAgent2::handleGotoPlPeak(Action & a)
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
		d_pl->getPos( id, pos, (d_spec3D && d_show3DPlane)?d_spec3D:d_spec2D );
		PpmPoint p;
		p.assign( Root::Math::min( d_pl->getDimCount(), (d_show3DPlane)?3:2 ), 0 );
		for( Dimension d = 0; d < p.size(); d++ )
			p[d] = pos[d];
		setCursor( p );
        centerToCursor( p.size() > 2 );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Goto Peak", e.what() );
	}
}

void PolyScopeAgent2::handleViewPlLabels(Action & a)
{
	if( a.getParamCount() == 0 )
		return;

	PeakPlaneView::Label q = (PeakPlaneView::Label) a.getParam( 0 ).getShort();
	if( q < PeakPlaneView::NONE || q >= PeakPlaneView::END )
		return;

	ACTION_CHECKED_IF( a, true,
		d_plane.d_peaks->getLabel() == q );
	
	d_plane.d_peaks->setLabel( q );
	for( int i = 0; i < d_strips.size(); i++ )
	{
		d_strips[i].d_peaks->setLabel( q );
		d_strips[i].d_viewer->redraw();
	}
	d_plane.d_viewer->redraw();
}

void PolyScopeAgent2::handleSyncDepth(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_syncDepth );
	
	d_syncDepth = !d_syncDepth;
	// Unterfeature von SyncCursor
}

void PolyScopeAgent2::handleAdjustIntensity(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	
	Dlg::adjustIntensity( MW, d_plane.d_intens );
}




