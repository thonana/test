#pragma once

#include <wx/app.h>

class App : public wxApp
{
public:
    virtual bool OnInit() override;
    virtual int  OnExit() override;
};

wxDECLARE_APP(App);