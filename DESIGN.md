# Mycolotchi — Design Spec

## Concept

A tamagotchi built around mushroom ecology. The player tends a sentient oyster mushroom, managing its environment to keep it healthy and growing. Species is fixed as oyster mushroom for v1.

The core loop is roguelike: the **mycelium network** is the persistent meta-layer. **Fruiting bodies** grow from it, mature, and die back naturally. When a flush ends the mycelium survives, and the next flush begins without starting from zero.

---

## Stats

### Substrate `0–100` — sweet-spot, fog-of-war discovery
The growing medium (straw, hardwood, etc.). Needs regular replenishment as the mycelium consumes it. Too little stunts growth; over-supplementation increases contamination risk.

- **Raised by:** feed action (add growing medium)
- **Decays:** over time
- **Sweet spot:** discovered through experimentation (player is an amateur, optimal quantity is not obvious)
- **Small deviation:** happiness decay only
- **Large deviation:** health + happiness decay

### Moisture `0–100` — sweet-spot, fog-of-war discovery
Humidity level. Oyster mushrooms need high but not excessive moisture. Ideal range is species-dependent and learned through experimentation.

- **Raised by:** mist action
- **Decays:** over time
- **Sweet spot:** discovered through experimentation
- **Small deviation:** happiness decay only
- **Large deviation:** health + happiness decay
- **Future:** purchasable environment elements maintain moisture passively

**Both substrate and moisture use the same fog-of-war meter mechanic:**
Regions of the meter the stat hasn't spent time in are obscured. As the value sits at a given level, that region clears and reveals whether it is harmful, neutral, or ideal. Once both the upper and lower bounds of the ideal range have been revealed, the fog lifts entirely.

### Happiness `0–100` — visible, standard
The mushroom's mood. Affected by environmental conditions and interaction.

- **Raised by:** mini-game
- **Decays:** over time
- **Also decays:** from substrate or moisture deviations

### Health `0–100` — **hidden**
Background vitality. Not shown on the stat screen. Low health increases the probability of a sick event. Managed indirectly through good environmental conditions.

- **Decays:** when substrate or moisture are in bad ranges
- **Recovers:** slowly and passively when both substrate and moisture are in good ranges
- **Low health:** probabilistic sick events (higher probability the lower health is)

### Mycelium `0–∞` — visible, unbounded
The mycelium network. Grows through physical activity and drives progression. Larger networks generate spores faster and unlock growth stages.

- **Raised by:** steps (walking)
- **Decays:** proportionally to current level (larger networks need more maintenance) — motivates staying active
- **Floor:** once established, never decays fully (mycelium is resilient)
- **Drives:** spore generation rate, growth stage thresholds

### Spores — visible, currency
Generated passively by the mycelium. The primary currency. Collecting them is the main alternative to grinding the mini-game.

- **Generated:** passively at a rate proportional to mycelium level
- **Collected:** tap to collect when they appear on screen
- **Spent:** in shop (treatment items, environment upgrades, autocollector, etc.)

---

## Sick Mechanic

- Low health creates a **probabilistic sick event** — the lower health is, the more likely sickness occurs
- Sick status is **visible**: the fruiting body appears diseased
- **Treatment item** (purchasable with spores): clears sick status and restores health enough to prevent immediate re-sickness
- Health recovers **slowly and passively** when substrate and moisture are both in their ideal ranges

---

## Growth Stages (Oyster Mushroom)

Triggered by mycelium crossing thresholds. Senescence ends the flush; the mycelium persists and the cycle restarts from Stage 0.

| Stage | Name | Notes |
|---|---|---|
| 0 | Colonized | Starting state — established network, no fruiting body |
| 1 | Pinning | Tiny pins visible on substrate surface |
| 2 | Young Fruiting Body | Recognizable oyster shape, small |
| 3 | Mature | Full size, peak spore production |
| — | Senescence | Natural flush end or health collapse; fruiting body dies, mycelium persists |

---

## Shop (planned)

Purchasable with spores:
- **Treatment item** — clears sick status, restores health
- **Autocollector** — collects spores automatically
- **Humidity elements** — passively raise/maintain moisture
- Additional environment upgrades TBD

---

## Implementation Notes

- All numerical thresholds (sweet spot ranges, mycelium decay rate, growth stage thresholds, sick probability curve) are intentionally prototyping values — expect significant tuning to make the game feel right.
- Health is never displayed directly; only its consequences (sick status) are visible.
- The moisture sweet spot reveal mechanic needs a discovery condition (TBD — time in range? explicit trigger?).
- **When switching status bars to discrete icons:** `kFogSegments` in `systems.h` currently drives both the fog-of-war bitmask (data) and the rendered segment count (display). Before implementing icons, decouple these into two constants: `kFogSegments` (stays in `systems.h`, controls bitmask precision) and `kFogIconCount` (in `status_scene.h`, controls display). Icon reveals when any of its underlying fog buckets is visited. Current value of 10 was chosen so the ideal ranges (30 units wide) align cleanly to 3 buckets each.
