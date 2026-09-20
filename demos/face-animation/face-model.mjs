const PRESETS = Object.freeze({
  idle:       { lid: 0,  brow: 0,   mouth: 'soft',    accent: 'none',  tint: '#ffd85a' },
  happy:      { lid: 8,  brow: -7,  mouth: 'smile',   accent: 'cheek', tint: '#ffd85a' },
  curious:    { lid: 0,  brow: -12, mouth: 'o',       accent: 'spark', tint: '#ffe57c' },
  excited:    { lid: 0,  brow: -15, mouth: 'open',    accent: 'spark', tint: '#fff08a' },
  nervous:    { lid: 10, brow: 8,   mouth: 'wobble',  accent: 'sweat', tint: '#ffc95c' },
  sleepy:     { lid: 48, brow: 7,   mouth: 'soft',    accent: 'zzz',   tint: '#e8cf72' },
  reverse:    { lid: 15, brow: 12,  mouth: 'wobble',  accent: 'none',  tint: '#f4f5f2' },
  lowBattery: { lid: 24, brow: 9,   mouth: 'flat',    accent: 'battery', tint: '#ffb85a' },
  linkLost:   { lid: 38, brow: 15,  mouth: 'flat',    accent: 'signal', tint: '#ff8f82' },
});

export function moodPreset(name) {
  return { ...(PRESETS[name] ?? PRESETS.idle) };
}

const clamp = (value, min, max) => Math.min(max, Math.max(min, value));

export function batteryModel(value) {
  const percent = Math.round(clamp(Number(value) || 0, 0, 100));
  let segments = 0;
  if (percent > 85) segments = 6;
  else if (percent > 70) segments = 5;
  else if (percent > 55) segments = 4;
  else if (percent > 40) segments = 3;
  else if (percent > 25) segments = 2;
  else if (percent > 10) segments = 1;
  return { percent, segments, low: percent <= 10 };
}

export function batteryFromVoltage(voltage, cellCount = 3) {
  const cells = Number(cellCount);
  const volts = Number(voltage);
  if (!Number.isFinite(volts) || volts <= 0 || !Number.isInteger(cells) || cells < 2 || cells > 6) {
    return { percent: 0, segments: 0, low: false };
  }
  const perCell = volts / cells;
  const percent = perCell <= 3.5 ? 10 : perCell >= 4.2 ? 100
    : Math.round(10 + ((perCell - 3.5) * 90) / 0.7);
  return batteryModel(percent);
}

export function computeFace({ mood = 'idle', throttle = 0, steering = 0, time = 0 } = {}) {
  const speed = Math.abs(clamp(Number(throttle) || 0, -1, 1));
  const turn = clamp(Number(steering) || 0, -1, 1);
  const preset = moodPreset(mood);
  const excitement = mood === 'excited' ? 1 : mood === 'happy' ? 0.35 : 0;
  const growth = speed * 16 + excitement * 5;
  const shakeStrength = speed * (mood === 'nervous' ? 6 : 4);
  const shakeX = Math.sin(time * 0.045) * shakeStrength;
  const shakeY = Math.cos(time * 0.036) * shakeStrength * 0.55;
  const blinkPhase = time % 4400;
  const blink = preset.lid < 30 && blinkPhase >= 4200 && blinkPhase <= 4300
    ? Math.sin(((blinkPhase - 4200) / 100) * Math.PI) * 92
    : 0;

  return {
    ...preset,
    lid: Math.max(preset.lid, Math.round(blink)),
    eyeSize: {
      width: Math.round(94 + growth),
      height: Math.round(104 + growth),
    },
    pupilSize: Math.round(42 + speed * 9 + excitement * 4),
    gazeX: Math.round(turn * 22),
    gazeY: Math.round(-speed * 7),
    shakeX: Math.round(shakeX * 100) / 100,
    shakeY: Math.round(shakeY * 100) / 100,
    tilt: turn * 3,
  };
}
