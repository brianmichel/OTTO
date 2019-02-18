#include "goss.hpp"

#include "core/ui/vector_graphics.hpp"

namespace otto::engines {

  using namespace ui;
  using namespace ui::vg;

  /*
   * Declarations
   */

  struct GossSynthScreen : EngineScreen<GossSynth> {
    void draw(Canvas& ctx) override;
    bool keypress(Key key) override;
    void rotary(RotaryEvent e) override;

    using EngineScreen<GossSynth>::EngineScreen;
  };

  // GossSynth ////////////////////////////////////////////////////////////////

  GossSynth::GossSynth()
    : SynthEngine("Goss", props, std::make_unique<GossSynthScreen>(this)), voice_mgr_(props)
  {}

  float GossSynth::Voice::operator()() noexcept
  {
    float fundamental = frequency() * (1 + 0.02 * props.leslie * pre.pitch_modulation_hi.cos());
    pipes[0].freq(fundamental);
    pipes[1].freq(frequency()/2);
    pipes[2].freq(fundamental * 1.334839854); // A fifth above fundamental
    pipes[3].freq(fundamental * 2);
    return pipes[0]() + pipes[1]() * props.drawbar1 + pipes[2]() * props.drawbar2 + pipes[3]() * props.drawbar3;
  }

  GossSynth::Voice::Voice(Pre& pre) noexcept : VoiceBase(pre) {
    pipes[0].resize(512);
    pipes[0].addSine(1, 1, 0);
    for (int i=1; i!=3; i++) {
      pipes[i].source(pipes[0]);
    }
  }

  GossSynth::Pre::Pre(Props& props) noexcept : PreBase(props)
  {
    leslie_filter_hi.phase(0.5);
    leslie_filter_lo.phase(0.5);
    props.leslie.on_change().connect([this](float leslie) {
      leslie_speed_lo = leslie * 10;
      leslie_speed_hi = leslie * 3;
      leslie_filter_hi.freq(leslie_speed_hi);
      leslie_filter_lo.freq(leslie_speed_lo);
      leslie_amount_hi = leslie * 0.3;
      leslie_amount_lo = leslie * 0.5;
      pitch_modulation_lo.freq(leslie_speed_hi);
      pitch_modulation_hi.freq(leslie);
      rotation.freq(leslie_speed_hi/4.f);
    });
  }

  void GossSynth::Pre::operator()() noexcept {
    props.rotation_value = rotation.nextPhase();
  }

  /// Constructor. Takes care of linking appropriate variables to props
  GossSynth::Post::Post(Pre& pre) noexcept : PostBase(pre)
  {
    lpf.type(gam::LOW_PASS);
    lpf.freq(1800);
    lpf.res(1);
    hpf.type(gam::HIGH_PASS);
    hpf.freq(1800);
    hpf.res(1);
  }

  float GossSynth::Post::operator()(float in) noexcept
  {
    float s_lo = lpf(in) * (1 + pre.leslie_amount_lo * pre.leslie_filter_lo.cos());
    float s_hi = hpf(in) * (1 + pre.leslie_amount_hi * pre.leslie_filter_hi.cos());
    return s_lo + s_hi;
  }

  audio::ProcessData<1> GossSynth::process(audio::ProcessData<1> data)
  {
    return voice_mgr_.process(data);
  }

  /*
   * GossSynthScreen
   */

  bool GossSynthScreen::keypress(Key key)
  {
    return false;
  }

  void GossSynthScreen::rotary(RotaryEvent e)
  {
    switch (e.rotary) {
    case Rotary::blue: engine.props.drawbar1.step(e.clicks); break;
    case Rotary::green: engine.props.drawbar2.step(e.clicks); break;
    case Rotary::yellow: engine.props.drawbar3.step(e.clicks); break;
    case Rotary::red: engine.props.leslie.step(e.clicks); break;
    }
  }

  void GossSynthScreen::draw(ui::vg::Canvas& ctx)
  {
    using namespace ui::vg;

    ctx.font(Fonts::Norm, 35);

    // Gray Base Layers
    ctx.group([&] {
      // Ring 1 Base
      ctx.beginPath();
      ctx.circle({160, 120}, 55);
      ctx.lineWidth(6.0);
      ctx.strokeStyle(Colours::Gray50);
      ctx.lineCap(Canvas::LineCap::ROUND);
      ctx.lineJoin(Canvas::LineJoin::ROUND);
      ctx.stroke();

      // Ring 2 Base
      ctx.beginPath();
      ctx.circle({160, 120}, 75);
      ctx.stroke();

      // Ring 3 Base
      ctx.beginPath();
      ctx.circle({160, 120}, 95);
      ctx.stroke();
    });

    // Coloured Parameters
    ctx.group([&] {
      // Ring 1
      ctx.beginPath();
      ctx.rotateAround(55, {160, 120});
      ctx.arc(160, 120, 55, 0, (2 * M_PI * engine.props.drawbar3), false);
      ctx.lineWidth(6.0);
      ctx.strokeStyle(Colours::Yellow);
      ctx.lineCap(Canvas::LineCap::ROUND);
      ctx.lineJoin(Canvas::LineJoin::ROUND);
      ctx.stroke();

      // Ring 2
      ctx.beginPath();
      ctx.arc(160, 120, 75, 0, (2 * M_PI * engine.props.drawbar2), false);
      ctx.strokeStyle(Colours::Green);
      ctx.stroke();

      // Ring 3
      ctx.beginPath();
      ctx.arc(160, 120, 95, 0, (2 * M_PI * engine.props.drawbar1), false);
      ctx.strokeStyle(Colours::Blue);
      ctx.stroke();
    });

    // middle red ring
    ctx.group([&] {
      // Ring Base
      ctx.beginPath();
      ctx.lineWidth(6.0);
      ctx.strokeStyle(Colours::Red);

      ctx.rotateAround(engine.props.rotation_value, {160, 120});
      ctx.circle({160, height/2 + engine.props.leslie*25}, 12.5);
      ctx.stroke();

      ctx.circle({160, height / 2 + engine.props.leslie * 25}, 12.5);
      ctx.stroke();
    });
    ///
  }
} // namespace otto::engines