//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// copyright            : (C) 2014 Eran Ifrah
// file name            : cl_aui_dock_art.cpp
//
// -------------------------------------------------------------------------
// A
//              _____           _      _     _ _
//             /  __ \         | |    | |   (_) |
//             | /  \/ ___   __| | ___| |    _| |_ ___
//             | |    / _ \ / _  |/ _ \ |   | | __/ _ )
//             | \__/\ (_) | (_| |  __/ |___| | ||  __/
//              \____/\___/ \__,_|\___\_____/_|\__\___|
//
//                                                  F i l e
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include "ColoursAndFontsManager.h"
#include "clStatusBar.h"
#include "clSystemSettings.h"
#include "clTabRenderer.h"
#include "cl_aui_dock_art.h"
#include "cl_command_event.h"
#include "cl_config.h"
#include "codelite_events.h"
#include "drawingutils.h"
#include "event_notifier.h"
#include "globals.h"
#include "imanager.h"
#include <editor_config.h>
#include <wx/dcmemory.h>
#include <wx/settings.h>
#include <wx/xrc/xmlres.h>

// --------------------------------------------
static bool IsRectOK(wxDC& dc, const wxRect& rect)
{
    const wxSize dc_size = dc.GetSize();

    if(0 > rect.x || 0 > rect.y || 0 >= rect.width || 0 >= rect.height || dc_size.GetWidth() < (rect.x + rect.width) ||
       dc_size.GetHeight() < (rect.y + rect.height))
        return (false);
    return (true);
}

static wxString wxAuiChopText(wxDC& dc, const wxString& text, int max_size)
{
    wxCoord x, y;

    // first check if the text fits with no problems
    dc.GetTextExtent(text, &x, &y);
    if(x <= max_size)
        return text;

    size_t i, len = text.Length();
    size_t last_good_length = 0;
    for(i = 0; i < len; ++i) {
        wxString s = text.Left(i);
        s += wxT("...");

        dc.GetTextExtent(s, &x, &y);
        if(x > max_size)
            break;

        last_good_length = i;
    }

    wxString ret = text.Left(last_good_length);
    ret += wxT("...");
    return ret;
}

static void clDockArtGetColours(wxColour& bgColour, wxColour& penColour, wxColour& textColour)
{
#ifdef __WXMSW__
    bgColour = clSystemSettings::GetColour(wxSYS_COLOUR_3DFACE).ChangeLightness(95);
#else
    bgColour = clSystemSettings::GetColour(wxSYS_COLOUR_WINDOW).ChangeLightness(95);
#endif
    if(!DrawingUtils::IsDark(bgColour)) {
        textColour = wxColour(*wxBLACK).ChangeLightness(130);
    } else {
        textColour = wxColour(*wxWHITE).ChangeLightness(70);
    }

    penColour = bgColour;
}

static wxColour clDockArtSashColour()
{
    wxColour baseColour = DrawingUtils::GetMenuBarBgColour(true);
    return baseColour;
}

// ------------------------------------------------------------

clAuiDockArt::clAuiDockArt(IManager* manager)
    : m_manager(manager)
{
    EventNotifier::Get()->Bind(wxEVT_SYS_COLOURS_CHANGED, &clAuiDockArt::OnSettingsChanged, this);
    m_captionColour = DrawingUtils::GetCaptionColour();
    m_captionTextColour = DrawingUtils::GetCaptionTextColour();
    m_useCustomCaptionColour = clConfig::Get().Read("UseCustomCaptionsColour", false);

    m_bgColour = DrawingUtils::GetPanelBgColour();
    EventNotifier::Get()->Bind(wxEVT_CMD_COLOURS_FONTS_UPDATED, [&](clCommandEvent& event) {
        event.Skip();
        m_bgColour = DrawingUtils::GetPanelBgColour();
        bool useCustomColour = clConfig::Get().Read("UseCustomBaseColour", false);
        if(useCustomColour) {
            m_bgColour = clConfig::Get().Read("BaseColour", m_bgColour);
        }

        // Trigger a refresh
        m_manager->GetDockingManager()->Update();
    });
}

clAuiDockArt::~clAuiDockArt()
{
    EventNotifier::Get()->Unbind(wxEVT_SYS_COLOURS_CHANGED, &clAuiDockArt::OnSettingsChanged, this);
}

void clAuiDockArt::DrawPaneButton(wxDC& dc, wxWindow* window, int button, int button_state, const wxRect& _rect,
                                  wxAuiPaneInfo& pane)
{
    wxRect buttonRect = _rect;

    if(!IsRectOK(dc, _rect))
        return;
    // Make sure that the height and width of the button are equals
    if(buttonRect.GetWidth() != buttonRect.GetHeight()) {
        buttonRect.SetHeight(wxMin(buttonRect.GetHeight(), buttonRect.GetWidth()));
        buttonRect.SetWidth(wxMin(buttonRect.GetHeight(), buttonRect.GetWidth()));
    } else {
        buttonRect.Deflate(1);
    }
    if(buttonRect.GetHeight() < 2) {
        // A wx3.0.x build may arrive here with a 1*1 rect -> a sigabort in libcairo
        return;
    }
    buttonRect = buttonRect.CenterIn(_rect);
    eButtonState buttonState = eButtonState::kNormal;
    switch(button_state) {
    case wxAUI_BUTTON_STATE_HOVER:
        buttonState = eButtonState::kHover;
        break;
    case wxAUI_BUTTON_STATE_PRESSED:
        buttonState = eButtonState::kPressed;
        break;
    case wxAUI_BUTTON_STATE_NORMAL:
    default:
        buttonState = eButtonState::kNormal;
        break;
    }

    // Prepare the colours
    wxColour bgColour, penColour, textColour;
    clDockArtGetColours(bgColour, penColour, textColour);

    switch(button) {
    case wxAUI_BUTTON_CLOSE:
        DrawingUtils::DrawButtonX(dc, window, buttonRect, penColour, bgColour, buttonState);
        break;
    case wxAUI_BUTTON_MAXIMIZE_RESTORE:
        DrawingUtils::DrawButtonMaximizeRestore(dc, window, buttonRect, penColour, bgColour, buttonState);
        break;
    default:
        // Make sure that the pane buttons are drawn with proper colours
        pane.state |= wxAuiPaneInfo::optionActive;
        wxAuiDefaultDockArt::DrawPaneButton(dc, window, button, button_state, _rect, pane);
        break;
    }
}

void clAuiDockArt::DrawCaption(wxDC& dc, wxWindow* window, const wxString& text, const wxRect& rect,
                               wxAuiPaneInfo& pane)
{
    wxRect tmpRect;
    window->PrepareDC(dc);

    if(!IsRectOK(dc, rect))
        return;

#ifdef __WXOSX__
    tmpRect = rect;
    tmpRect.Inflate(1);

    // Prepare the colours
    wxFont f = DrawingUtils::GetDefaultGuiFont();
    dc.SetFont(f);

    wxColour captionBgColour, penColour, textColour;
    clDockArtGetColours(captionBgColour, penColour, textColour);

    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(clSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    dc.DrawRectangle(tmpRect);

    int caption_offset = 5;
    dc.SetTextForeground(textColour);
    wxCoord w, h;
    dc.GetTextExtent(wxT("ABCDEFHXfgkj"), &w, &h);

    wxRect clip_rect = tmpRect;
    clip_rect.width -= 3; // text offset
    clip_rect.width -= 2; // button padding
    if(pane.HasCloseButton()) {
        clip_rect.width -= m_buttonSize;
    }
    if(pane.HasPinButton()) {
        clip_rect.width -= m_buttonSize;
    }
    if(pane.HasMaximizeButton()) {
        clip_rect.width -= m_buttonSize;
    }

    wxString draw_text = wxAuiChopText(dc, text, clip_rect.width);
    wxSize textSize = dc.GetTextExtent(draw_text);
    dc.DrawText(draw_text, tmpRect.x + 3 + caption_offset, tmpRect.y + ((tmpRect.height - textSize.y) / 2));
#else
    tmpRect = rect;
    tmpRect.SetPosition(wxPoint(0, 0));
    wxBitmap bmp(tmpRect.GetSize());
    {
        wxMemoryDC memDc;
        memDc.SelectObject(bmp);

        wxGCDC gdc;
        wxDC* pDC = NULL;
        if(!DrawingUtils::GetGCDC(memDc, gdc)) {
            pDC = &memDc;
        } else {
            pDC = &gdc;
        }

        wxFont f = DrawingUtils::GetDefaultGuiFont();
        pDC->SetFont(f);

        wxColour captionBgColour, penColour, textColour;
        clDockArtGetColours(captionBgColour, penColour, textColour);

        // we inflat the rect by 1 to fix a one pixel glitch
        pDC->SetPen(*wxTRANSPARENT_PEN);
        pDC->SetBrush(clSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
        tmpRect.Inflate(1);
        pDC->DrawRectangle(tmpRect);

        int caption_offset = 5;

        wxRect clip_rect = tmpRect;
        clip_rect.width -= caption_offset; // text offset
        clip_rect.width -= 2;              // button padding
        if(pane.HasCloseButton())
            clip_rect.width -= m_buttonSize;
        if(pane.HasPinButton())
            clip_rect.width -= m_buttonSize;
        if(pane.HasMaximizeButton())
            clip_rect.width -= m_buttonSize;

        // Truncate the text if needed
        wxString draw_text = wxAuiChopText(gdc, text, clip_rect.width);
        wxSize textSize = pDC->GetTextExtent(draw_text);
        wxRect textRect(textSize);
        textRect = textRect.CenterIn(clip_rect, wxVERTICAL);
        textRect.SetX(caption_offset);

        pDC->SetTextForeground(textColour);
        pDC->DrawText(draw_text, textRect.GetTopLeft());
        memDc.SelectObject(wxNullBitmap);
    }
    dc.DrawBitmap(bmp, rect.x, rect.y, true);
#endif
}

void clAuiDockArt::DrawBackground(wxDC& dc, wxWindow* window, int orientation, const wxRect& rect)
{
    dc.SetBrush(clSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    dc.SetPen(clSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    dc.DrawRectangle(rect);
    // return wxAuiDefaultDockArt::DrawBackground(dc, window, orientation, rect);
}

void clAuiDockArt::DrawBorder(wxDC& dc, wxWindow* window, const wxRect& rect, wxAuiPaneInfo& pane)
{
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(clSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    dc.DrawRectangle(rect);
    // return wxAuiDefaultDockArt::DrawBorder(dc, window, rect, pane);
}

void clAuiDockArt::DrawSash(wxDC& dc, wxWindow* window, int orientation, const wxRect& rect)
{
    wxUnusedVar(orientation);
    wxUnusedVar(window);

    wxColour c = clDockArtSashColour();
    dc.SetPen(c);
    dc.SetBrush(c);
    dc.DrawRectangle(rect);
}

void clAuiDockArt::OnSettingsChanged(clCommandEvent& event)
{
    event.Skip();
    clColours colours;
    wxColour baseColour = colours.GetBgColour();
    bool useCustomColour = clConfig::Get().Read("UseCustomBaseColour", false);
    if(useCustomColour) {
        baseColour = clConfig::Get().Read("BaseColour", baseColour);
    } else {
        // we use the *native* background colour (notice that we are using wxSystemSettings)
        baseColour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    }
    colours.InitFromColour(baseColour);

    m_captionColour = clSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION);
    m_captionTextColour = colours.GetItemTextColour();
    m_bgColour = clSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    m_useCustomCaptionColour = false;
}
