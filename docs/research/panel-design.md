# What makes or breaks a plugin panel (VS1-06)

Research of 2026-11-05, for the panel of FAST-05. Web only, by three research agents; nobody tried these plugins.

**How far it carries.** The evidence is thinner than the subject. No study of on-screen knobs was found, no designer states
a knob diameter or a minimum text size, and Reddit and Gearspace gave almost nothing. Several designer interviews could not
be read (Valhalla's blog returned 403, so that point rests on a search summary). Tags: **[many]**, **[several]**, **[one]**
for how many voices; **[designer]**, **[reviewer]**, **[forum]** for who; **[inference]** for our own reasoning. The
visual rules at the end are mostly taste with a reason behind it, and they are the owner's to overrule.

## What breaks a plugin

- **Text you cannot read, and scaling that does not carry it** [many, forum]. Kick 3 owners report grey-on-black numbers they
  cannot read and a pixelated window. Labels that do not scale with knobs recur across vendors.
  [KVR](https://www.kvraudio.com/forum/viewtopic.php?p=8984433),
  [Sonic Academy](https://forums.sonicacademy.com/t/how-do-i-resize-kick-3s-interface/66857),
  [NI](https://community.native-instruments.com/discussion/169/please-make-all-uis-scalable/p11),
  [AnalogTapeModel](https://github.com/jatinchowdhury18/AnalogTapeModel/issues/259)
- **Low contrast, unclear inactive states, too many sizes and colours** [one, forum, specific].
  [KVR open letter](https://www.kvraudio.com/forum/viewtopic.php?p=8700079)
- **A first view that shouts** [one, forum, specific]. On Audija KickDrum: a waveform edge to edge, text boxes on every node,
  a side bar of 20 buttons. [KVR](https://www.kvraudio.com/forum/viewtopic.php?p=9005524)
- **Hidden state** [several, reviewer]. Massive X and Serum 2 are faulted for what sits behind pages and menus.
  [Sound On Sound](https://www.soundonsound.com/reviews/native-instruments-massive-x?page=2)
- **A window too large** [one, reviewer]. It hides the arrangement, which annoys more than a small window.
  [Production Expert](https://www.production-expert.com/production-expert-1/plugin-designs-why-we-love-some-and-hate-others)
- **A GUI that eats CPU** [several, forum]. Vital spikes with the editor open. Repaint only what changes, from one timer; 30 Hz
  is usually enough. [Vital](https://forum.vital.audio/t/high-cpu-usage-when-gui-is-open/3016),
  [JUCE](https://forum.juce.com/t/frequent-repaint-and-cpu-usage/11108)
- **No one knob for the obvious thing** [one, reviewer]. Chow Kick: decay comes from several interacting controls, and body pitch
  is not in notes. [KVR](https://www.kvraudio.com/product/chow-kick-by-chowdhury-dsp/reviews)

## What wins people over

- **One window, no menu-diving** [several, reviewer]. Sylenth1 and Phase Plant. But Sylenth1's fixed size and jargon are its costs.
  [MusicRadar](https://musicradar.com/reviews/tech/lennardigital-sylenth1-22935), [Dubspot](https://blog.dubspot.com/es/plugins/sylenth1)
- **State you can see on the knob itself** [many]. Vital and Serum 2's rings, Pigments' colours.
  [MusicTech](https://musictech.com/reviews/arturia-pigments-review/)
- **A phase you can trust** [several]. Kick 3's tail lock is praised. Kikkuh's Tail Anchor is the same idea, so it should be visible.
  [MusicTech](https://musictech.com/reviews/plug-ins/sonic-academy-kick-3-review/)
- **Labels that explain themselves** [designer]. Valhalla wants self-documenting controls. [Valhalla](https://valhalladsp.com/?p=149652)

## Working on a kick

Most reached for [inference, from tutorials]: pitch drop amount and time, body pitch, length, then drive.
[Gearnews](https://www.gearnews.com/?p=246014) Users want a readable first 10 ms [several, forum], which is CURV-01 in slice 5.
[KVR](https://www.kvraudio.com/forum/viewtopic.php?p=8969350)

Nothing was found that argues for tabs in a kick synth. A single old review defends Serum's tabs because screens are small. So
at 50 knobs the real limit is scaling and text size [inference].

## How a knob should behave

| What | Convention | Source |
| --- | --- | --- |
| Drag | vertical; not circular | [FabFilter](https://www.fabfilter.com/help/pro-q/using/knobs), [forum](https://lists.linuxaudio.org/pipermail/linux-audio-dev/2010-September/028229.html) |
| Fine | Shift + drag or scroll, at a fixed ratio; switching mid-gesture must not jump | FabFilter, [Phase Plant](https://cdn.shopify.com/s/files/1/1793/8985/files/phase_plant.pdf), [JUCE](https://forum.juce.com/t/implementing-a-true-fine-mode-for-my-sliders/37730) |
| Reset and typing | **Split:** FabFilter has Cmd-click reset, double-click type. Kilohearts has double-click reset, right-click type. KVR asks for double-click reset | FabFilter, Phase Plant, [KVR](https://kvraudio.com/forum/viewtopic.php?p=8862436) |
| Typing | accepts units and notes: "1k", "A4" | FabFilter |
| Scroll | on hover | FabFilter |
| Hover | the knob answers, and shows name and value | FabFilter |

Choice for Kikkuh [inference]: double-click and Cmd-click both reset, and clicking the value text types. Nothing conflicts, and
the value is the thing you want to type into.

## Sizes and scale

No source gives a number. Contrast has a web standard: 4.5:1 for text ([WCAG](https://www.w3.org/WAI/WCAG22/Understanding/contrast-minimum.html));
we use it as a floor [inference]. Serum 2 lets you drag the corner and save a size as default
([Xfer](https://xferrecords.com/web-manual/serum-2/resizing-the-ui)). One factor scales text and knobs together [inference].

## Looks

- **What is criticised, with a source:** screws, rack ears and brushed metal ([Linux audio list](https://lists.linuxaudio.org/hyperkitty/list/linux-audio-dev@lists.linuxaudio.org/message/DIXUFVX2FA2USUUMCHK6SA2XH5XLONJT), niche),
  skeuomorphic excess ([Boing Boing](https://boingboing.net/2017/08/25/the-skeuomorphic-hell-of-music.html/amp), 2017), low contrast (KVR letter).
  Some users do want photoreal panels ([MOD](https://forum.mod.audio/t/skeuomorphism-in-plugin-uis-should-we-avoid-it/1267)).
- **What is only suspected:** no source was found against neon on black, LED rings, gradient knobs, orange on black, HUD looks or
  skulls in plugins. The case against them is by analogy with lists of AI tells.
- **AI tells that apply to a panel** ([hey.com](https://world.hey.com/kostac/spot-the-slop-a-ui-designer-s-guide-to-fixing-ai-defaults-4c448c9c),
  [impeccable.style](https://impeccable.style/slop); opinion, partly from tool vendors): neon and glow on dark, gradients, uniform
  radius and spacing, one default font, nested cards, generic labels, decorative animation. Web tells such as hero copy do not apply.
- **Point of view comes from one rule, applied every time** [inference]: Braun's colour only communicates
  ([Braun](https://www.braun-audio.com/en-ES/stories/design/braun-colour-choices/)); the MS-20 prints its signal flow on the panel
  ([listing](https://www.musicswopshop.com.au/products/korg-ms-20)); Valhalla sets the name large in one typeface;
  the 808 colours its rows of buttons ([commentary](https://madegooddesigns.com/roland-brand-guidelines/), rationale unconfirmed).
- **Techno's own graphic language** is monochrome, industrial, photocopied, stencilled
  ([Underground Resistance](https://www.fontsinuse.com/uses/48014/ur-electronic-warfare-designs-for-sonic-revol),
  [Berlin flyers](https://daily.redbullmusicacademy.com/2018/09/three-decades-of-berlin-club-flyers)). Not skulls.

## What this means for the panel

1. **Colour has one job.** The panel is paper and ink; one colour means "sounding". It lights on the pad when a kick plays and on
   the loop while it runs, nothing else.
2. **The layout prints the signal flow,** as the MS-20 does: a printed line from section to section. No boxes in boxes.
3. **Uneven on purpose.** Knobs people reach for most (Amount, Decay, length, Tune) are larger; related knobs sit closer.
4. **Tune is a note you can read,** with the tail readout beside it, large. This answers Chow Kick's complaint.
5. **Text never below the contrast floor,** and it scales with the knobs.
6. **Tail Anchor is visible and named,** as an on-panel state.
7. **Nothing animates unless the signal does.**
8. **Light panel as the first proposal.** A light panel avoids the near-black and neon that every list warns of, and
   low contrast is the one sourced complaint. It may be tiring in a dark studio and it is unusual; that is the owner's call, so
   the prototype offers both.
