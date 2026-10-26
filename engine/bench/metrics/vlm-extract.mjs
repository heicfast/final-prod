#!/usr/bin/env node
// vlm-extract.mjs - structured VLM extraction of og UI screenshots (local side).
//
// Protocol (benchmark mandate §3): fixed template, every value confidence-flagged,
// model/version recorded, cross-checked against machine-readable sources
// (compare_og_rsf.py output, DRWUI.log) by vlm_crosscheck.py on the analysis side.
//
// Usage: node vlm-extract.mjs <screenshots-dir> <out-json> [run-id]
//
// Licensed under the MIT license. See LICENSE.

import ZAI from 'z-ai-web-dev-sdk';
import fs from 'fs';
import path from 'path';

const TEMPLATE = `You are reading a screenshot of the EaseUS Data Recovery Wizard Free UI (Windows, 1024x768). Extract structured facts. For EVERY value, also state your confidence: high (text clearly readable), medium (partially readable/inferred), low (uncertain). Answer ONLY with JSON of this exact shape:
{
  "screen_kind": "landing|scanning|results_tree|destination_dialog|recovery_progress|completion|error_dialog|other",
  "file_count": {"value": <number|null>, "confidence": "high|medium|low"},
  "total_size": {"value": "<string|null>", "confidence": "high|medium|low"},
  "listed_filenames": {"value": ["<up to 10 names>"], "confidence": "high|medium|low"},
  "selected_filenames": {"value": ["<up to 10 names>"], "confidence": "high|medium|low"},
  "recovered_count": {"value": <number|null>, "confidence": "high|medium|low"},
  "error_dialogs": {"value": ["<dialog title/text if any>"], "confidence": "high|medium|low"},
  "drive_or_location_labels": {"value": ["<labels>"], "confidence": "high|medium|low"},
  "notes": "<one line of anything anomalous>"
}`;

async function main() {
    const [dir, outPath, runId] = process.argv.slice(2);
    if (!dir || !outPath) {
        console.error('usage: node vlm-extract.mjs <screenshots-dir> <out-json> [run-id]');
        process.exit(2);
    }
    const files = fs.readdirSync(dir)
        .filter(f => f.toLowerCase().endsWith('.png'))
        .sort();
    const zai = await ZAI.create();
    const results = [];
    for (const f of files) {
        const b64 = fs.readFileSync(path.join(dir, f)).toString('base64');
        try {
            const resp = await zai.chat.completions.createVision({
                messages: [{
                    role: 'user',
                    content: [
                        { type: 'text', text: TEMPLATE },
                        { type: 'image_url', image_url: { url: `data:image/png;base64,${b64}` } },
                    ],
                }],
                thinking: { type: 'disabled' },
            });
            let content = resp.choices[0]?.message?.content || '';
            // strip markdown fences if present
            content = content.replace(/^```(json)?\s*/m, '').replace(/\s*```$/m, '').trim();
            let parsed = null;
            try { parsed = JSON.parse(content); } catch { parsed = { parse_error: true, raw: content.slice(0, 2000) }; }
            results.push({
                file: f, extraction: parsed,
                model: resp.model || 'unknown',
                extracted_utc: new Date().toISOString(),
            });
            console.log(`extracted ${f}: kind=${parsed.screen_kind || '?'} parse=${parsed.parse_error ? 'FAILED' : 'ok'}`);
        } catch (e) {
            results.push({ file: f, error: String(e), extracted_utc: new Date().toISOString() });
            console.log(`FAILED ${f}: ${e}`);
        }
    }
    const out = {
        run_id: runId || 'unspecified',
        protocol: {
            prompt_template: TEMPLATE,
            note: 'fixed template; every value confidence-flagged; cross-check against .rsf-derived CSV + DRWUI.log happens in vlm_crosscheck',
        },
        screenshots: results,
    };
    fs.writeFileSync(outPath, JSON.stringify(out, null, 1));
    console.log(`wrote ${outPath} (${results.length} screenshots)`);
}
main().catch(e => { console.error(e); process.exit(1); });
