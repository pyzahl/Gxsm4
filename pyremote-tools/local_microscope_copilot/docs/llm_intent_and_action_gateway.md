# LLM Intent Mode And Parsed Action Gateway

This document describes how the local microscope copilot uses language models
without trusting free-form model text as a hardware controller.

## Safety Model

The language model is not the actuator.  Hardware-changing actions must pass
through deterministic code that:

- maps the request to a known microscope action,
- parses and validates physical units,
- converts values to GXSM4 base units,
- checks control level and arm/confirmation state,
- checks configured safety ranges,
- executes one explicit controller method, and
- reports the actual result.

The model may help interpret language, but it must never directly call GXSM4
or claim a hardware action occurred unless the deterministic action gateway
actually executed it.

## Chat Flow

`microscope_gradio_gui.py` currently uses this order in `chat()`:

1. `direct_safety_reply()` tries deterministic routing first.
2. If enabled, LLM Intent Mode asks the local model for strict JSON.
3. The returned JSON is reparsed by the deterministic router.
4. If no action path applies, the model may answer as advisory chat only.
5. Any free-form model reply that claims hardware action is corrected by
   `enforce_chat_action_boundary()`.

This order is intentional.  Clear commands such as `set bias to 0.2 V`, `start
scan`, `set offset X and Y to 0 A`, or `shift image 100 A left` should be
handled by deterministic parsing before the local model is involved.

## Deterministic Parser

The deterministic parser is the trusted microscope-command path.  It handles:

- readbacks such as `read bias`, `report scan geometry`, `show current`,
- Level 1 actions such as start/stop scan and bounded parameter writes,
- scan geometry: range, points, rotation,
- absolute offsets and relative image shifts,
- tip pulse/dip loading with exact execution confirmations,
- scan/tip analysis requests, and
- tip tune planner loop requests.

Physical values should include units unless they are true counts.  Examples:

- Voltage: `0.2 V`, `200 mV`, `1e-6 MV`.
- Current: `10 pA`, converted to `0.01 nA` for GXSM4.
- Length: `700 A`, `70 Ang`, `5 nm`.
- Time: `30 s`.
- Counts/points: `400`, `400 points`, `400 px`.
- Rotation: `90`, `90 deg`, or `90 degree`; unitless rotation means degrees.

Wrong dimensions are rejected.  For example, `1 Ang bias` must not become a
voltage.

## LLM Intent Mode

LLM Intent Mode is a constrained language-to-JSON adapter.  It asks the local
Ollama model to return exactly one JSON object:

```json
{
  "intent": "one_allowed_intent",
  "confidence": 0.0,
  "parameters": {},
  "rationale": "short"
}
```

Allowed intents are deliberately small:

- `set_parameter`
- `set_offsets_xy`
- `shift_image`
- `set_scan_geometry`
- `start_scan`
- `stop_scan`
- `read_parameter`
- `analyze_scan`
- `load_tip_pulse`
- `load_tip_dip`
- `no_action`

The JSON is not executed directly.  `execute_llm_intent()` maps it back into the
same deterministic functions used by normal chat.  It rejects low confidence,
unknown intents, non-object parameters, missing units, wrong units, and unsafe
values.

## Local Model Limits

Small local models can be useful for phrase normalization, but they often fail
at instrument-specific reasoning.  Known problems observed during testing:

- confusing `200 mV` with `200 V`,
- refusing allowed actions because it imagines it has no permissions,
- inventing placeholder readbacks such as `[value]`,
- giving generic SPM explanations instead of using GXSM4-specific state, and
- missing domain phrases such as `apply bias`, `fine tune tip`, or `take over
  running scan`.

For this reason, the deterministic parser should keep expanding for common
operator commands.  The local model should be treated as an optional helper,
not the source of truth.

## Action Levels

Action execution still depends on GUI control level and arming:

- Level 0: monitor/read-only/advisory.
- Level 1: normal bounded operations, such as start/stop scan, safe bias and
  current setpoint changes, scan geometry, scan speed, feedback parameters,
  and bounded GVP loads/execution.
- Level 2: advanced GVP and more aggressive tip tuning.
- Level 3: protected coarse/hyper actions, auto approach, and extreme
  procedures.

LLM Intent Mode does not bypass these levels.  If the GUI is not armed, or the
requested action exceeds the current level, nothing executes.

## Future Online/Larger LLM Use

A larger online model such as GPT can help when the local model fails, but it
should be used as a planner or interpreter, not as a direct hardware executor.

Recommended architecture:

1. Keep the deterministic parser as the first-pass controller.
2. When deterministic parsing fails, optionally send a sanitized context packet
   to the larger model.
3. Ask the larger model for a strict JSON plan, not prose.
4. Reparse every proposed action locally through the same unit parser and
   safety gates.
5. Show the proposed plan to the operator when the action is Level 2+ or
   ambiguous.
6. Execute only one local controller step at a time, then observe and replan.

The online model should receive only the minimum useful context:

- current control level and armed state,
- current known scan geometry and status,
- bounded summaries of image/tip/dFreq analysis,
- known hazards and selected candidate positions,
- the allowed action schema,
- the safety envelope and unit conventions.

It should not receive unnecessary local secrets, passwords, raw logs, or
unbounded data dumps.

## Suggested Online Planner Schema

Use a schema like this for larger-model planning:

```json
{
  "mode": "proposal_only",
  "goal": "operator goal in plain language",
  "confidence": 0.0,
  "risk_level": 0,
  "requires_operator_confirmation": true,
  "steps": [
    {
      "intent": "set_parameter|start_scan|analyze_scan|load_tip_pulse|load_tip_dip|move_tip_to_candidate|no_action",
      "parameters": {},
      "reason": "short domain-specific reason",
      "expected_observation": "what to check after this step"
    }
  ],
  "stop_conditions": [
    "condition that stops the plan"
  ]
}
```

Even for online planners, the local gateway should execute only supported
intents, reject invalid units, enforce control levels, and require explicit
operator confirmation for higher-risk actions.

## Implementation Notes

Primary code paths:

- `chat()` orchestrates deterministic, intent-mode, and advisory paths.
- `direct_safety_reply()` routes known requests before model use.
- `request_llm_intent()` calls Ollama `/api/chat` with `format=json`.
- `build_llm_intent_prompt()` defines the allowed local intent schema.
- `execute_llm_intent()` validates the JSON and maps it back to deterministic
  controller methods.
- `parse_si_quantity()` handles SI-style values and unit validation.
- `execute_chat_level1_action()` and `execute_chat_level1_plan()` enforce
  arming/control-level boundaries.

When adding new actions, prefer this order:

1. Add deterministic parser support and tests/smoke examples.
2. Add the intent name only if the parser alone is not enough.
3. Update the allowed intent prompt and this document.
4. Keep the final execution path shared with the deterministic parser.
