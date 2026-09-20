import assert from 'node:assert/strict';
import test from 'node:test';

import { batteryFromVoltage, batteryModel, computeFace, moodPreset } from './face-model.mjs';

test('idle face keeps round relaxed eyes', () => {
  const face = computeFace({ mood: 'idle', throttle: 0, steering: 0, time: 0 });

  assert.deepEqual(face.eyeSize, { width: 94, height: 104 });
  assert.equal(face.pupilSize, 42);
  assert.equal(face.shakeX, 0);
});

test('acceleration enlarges the eyes and adds a bounded lively shake', () => {
  const still = computeFace({ mood: 'idle', throttle: 0, steering: 0, time: 180 });
  const fast = computeFace({ mood: 'idle', throttle: 1, steering: 0, time: 180 });

  assert.ok(fast.eyeSize.width > still.eyeSize.width);
  assert.ok(fast.eyeSize.height > still.eyeSize.height);
  assert.ok(Math.abs(fast.shakeX) <= 4);
  assert.notEqual(fast.shakeX, 0);
});

test('steering moves both pupils without leaving the eye area', () => {
  const face = computeFace({ mood: 'curious', throttle: 0.4, steering: 1, time: 0 });

  assert.ok(face.gazeX > 0);
  assert.ok(face.gazeX <= 22);
});

test('mood presets provide distinct cute expressions', () => {
  const happy = moodPreset('happy');
  const sleepy = moodPreset('sleepy');
  const nervous = moodPreset('nervous');

  assert.equal(happy.mouth, 'smile');
  assert.ok(sleepy.lid > happy.lid);
  assert.equal(nervous.accent, 'sweat');
});

test('unknown mood safely falls back to idle', () => {
  assert.deepEqual(moodPreset('not-a-mood'), moodPreset('idle'));
});

test('relaxed moods blink periodically without changing sleepy lids', () => {
  const open = computeFace({ mood: 'idle', time: 0 });
  const blinking = computeFace({ mood: 'idle', time: 4250 });
  const sleepy = computeFace({ mood: 'sleepy', time: 4250 });

  assert.equal(open.lid, 0);
  assert.ok(blinking.lid >= 70);
  assert.equal(sleepy.lid, moodPreset('sleepy').lid);
});

test('battery maps the approved percentage boundaries to six segments', () => {
  const cases = [
    [0, 0], [10, 0], [11, 1], [25, 1], [26, 2], [40, 2],
    [41, 3], [55, 3], [56, 4], [70, 4], [71, 5], [85, 5],
    [86, 6], [100, 6],
  ];

  for (const [percent, segments] of cases) {
    assert.equal(batteryModel(percent).segments, segments, `${percent}%`);
  }
});

test('battery enters red flashing warning at ten percent or below', () => {
  assert.equal(batteryModel(10).low, true);
  assert.equal(batteryModel(11).low, false);
});

test('battery clamps malformed percentages to the display range', () => {
  assert.deepEqual(batteryModel(-20), { percent: 0, segments: 0, low: true });
  assert.deepEqual(batteryModel(140), { percent: 100, segments: 6, low: false });
});

test('3S demo derives the same battery state as firmware from voltage', () => {
  assert.deepEqual(batteryFromVoltage(12.6, 3), { percent: 100, segments: 6, low: false });
  assert.deepEqual(batteryFromVoltage(10.5, 3), { percent: 10, segments: 0, low: true });
  assert.equal(batteryFromVoltage(11.55, 3).segments, 3);
});
