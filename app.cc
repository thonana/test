#include "app.h"
#include "devicefocus.h"
#include "devicepump.h"
#include "deviceturret.h"
#include "ds/style.h"
#include "ds/ui/frame.h"

bool
App::OnInit()
{
    auto frame = new ds::Frame(nullptr);
    auto focus = new DeviceFocus();
    focus->Create(frame, wxID_ANY, "Focus-X");
    focus->SetStage(std::make_shared<ds::StageAgent>());
    auto turret = new DeviceTurret();
    turret->Create(frame, wxID_ANY, "Turret");
    auto pump = new DevicePump();
    pump->Create(frame, wxID_ANY, "Pump");

    frame->SetTitle("Device Control");
    frame->SetWindowStyle(wxCAPTION | wxCLOSE_BOX);
    frame->Add(focus, 0, wxEXPAND | wxALL, 5);
    frame->Add(turret, 0, wxEXPAND | wxALL, 5);
    frame->Add(pump, 0, wxEXPAND | wxALL, 5);

    frame->Show();

    ds::SystemStyle::Update();

    return true;
}

int
App::OnExit()
{
    return wxApp::OnExit();
}

wxIMPLEMENT_APP(App);