
#include "ds/async.h"
#include "ds/model.h"
#include "ds/ui/button.h"
#include "ds/ui/form.h"
#include "ds/ui/frame.h"
// #include <asio/experimental/awaitable_operators.hpp>
#include "ds/style.h"
#include <atomic>
#include <wx/app.h>
#include <wx/filepicker.h>
#include <wx/stattext.h>

// using namespace asio::experimental::awaitable_operators;

enum class TimerState
{
    idle,
    busy
};

class Timer : public ds::Model<Timer>
{
    asio::thread_pool  m_context;
    asio::steady_timer m_timer;
    // asio::steady_timer m_cancel;
    std::atomic_flag m_cancel;
    std::atomic_flag m_run;
    TimerState       m_state;
    std::string      m_time; // std::chrono::time_point<std::system_clock>

public:
    Timer()
      : m_context(1)
      , m_timer(m_context)
      //, m_cancel(m_context)
      , m_state(TimerState::idle)
      , m_time("...")
    {
        m_cancel.clear();
        m_run.clear();

        asio::co_spawn(m_context, Routine(), asio::detached);
    }

    ~Timer()
    {
        // m_cancel.cancel();
        // m_cancel.test_and_set();
        m_timer.cancel();
        m_context.join();

        m_run.clear();
    }

    void SetTimeText(const std::string& s)
    {
        m_time = s;
        Render();
    }

    const std::string GetTimeText() const { return m_time; }

    const TimerState& GetState() const { return m_state; }

    void ToggleState()
    {
        if (m_state == TimerState::busy) {
            m_state = TimerState::idle;
            m_run.clear();
        } else {
            m_state = TimerState::busy;
            m_run.test_and_set();
        }

        Render(); // ui update
    }

    void Render() override
    {
        m_timer.cancel();
        Model<Timer>::Render();
    }

private:
    // runs in a thread. use CallAfter
    asio::awaitable<void> Routine()
    {
        for (;;) {
            while (not m_cancel.test() && m_run.test()) {
                auto s = std::format("{:%T}", std::chrono::system_clock::now());
                CallAfter(&Timer::SetTimeText, s);

                m_timer.expires_after(std::chrono::milliseconds(10));
                try {
                    co_await m_timer.async_wait(asio::use_awaitable);
                } catch (std::system_error& e) {
                    if (e.code() == asio::error::operation_aborted) {
                        break;
                    }
                    throw;
                }
            }

            if (m_cancel.test()) {
                break;
            }

            m_timer.expires_after(std::chrono::microseconds(200));
            try {
                co_await m_timer.async_wait(asio::use_awaitable);
            } catch (std::system_error& e) {
                if (e.code() == asio::error::operation_aborted) {
                    break;
                }
                throw;
            }
        }

        // while (true) {
        //     if (m_run.test()) {
        //         auto s = std::format("{:%T}",
        //         std::chrono::system_clock::now());
        //         CallAfter(&Timer::SetTimeText, s);
        //     }

        //    m_timer.expires_after(std::chrono::milliseconds(10));
        //    // co_await m_timer.async_wait(asio::use_awaitable);
        //    try {
        //        co_await m_timer.async_wait(asio::use_awaitable);
        //    } catch (std::system_error& e) {
        //        if (e.code() == asio::error::operation_aborted) {
        //            break;
        //        }
        //        throw;
        //    }
        //}

        //
        co_return;
    }
};

class BasicDemo
  : public ds::Form
  , public ds::View<Timer>
{
    ds::Button*   m_start;
    wxStaticText* m_text;

    wxFilePickerCtrl* m_filepicker;

    std::shared_ptr<Timer> m_timer;

public:
    bool Create(wxWindow* parent)
    {
        if (not ds::Form::Create(parent, wxID_ANY, _("Basic Demo"))) {
            return false;
        }

        m_start = new ds::Button(GetStaticBox(), wxID_ANY, _("Start"));
        m_text  = new wxStaticText(GetStaticBox(), wxID_ANY, _("..."));
        m_text->SetMinSize({ 100, 20 });

        m_start->BindModel(&Timer::ToggleState, m_timer);
        AddRow(m_start, m_text);

        m_filepicker = new wxFilePickerCtrl(GetStaticBox(), wxID_ANY);
        m_filepicker->Bind(wxEVT_FILEPICKER_CHANGED,
                           [this](wxFileDirPickerEvent& event) {
                               std::filesystem::path path(event.GetPath());
                               ds::SystemStyle::Load(path);
                           });

        AddRow("Style", m_filepicker);

        return true;
    }

    void SetTimer(const std::shared_ptr<Timer>& timer)
    {
        m_timer = timer;
        m_timer->Register(this);
    }

    void Render(const Timer& timer) override
    {
        switch (timer.GetState()) {
            case TimerState::idle: {
                m_start->SetLabel("Start");
                break;
            }
            case TimerState::busy: {
                m_start->SetLabel("Stop");
                break;
            }
        }

        m_text->SetLabel(timer.GetTimeText());
    }
};

class BasicApp : public wxApp
{
public:
    bool OnInit() override
    {
        if (wxApp::OnInit()) {
            auto timer = std::make_shared<Timer>();

            auto frame = new ds::Frame(nullptr);
            auto t1    = new BasicDemo();
            auto t2    = new BasicDemo();

            t1->Create(frame);
            t2->Create(frame);
            t1->SetTimer(timer);
            t2->SetTimer(timer);

            frame->Add(t1);
            frame->Add(t2);
            frame->Show();

            ds::SystemStyle::Update();
            return true;
        }

        return false;
    }
};

wxIMPLEMENT_APP(BasicApp);