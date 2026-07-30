# Making AI Write My Code — comparison site

A static website presenting the Roadtrip Optimizer study: one hand-specified
project and three AI renditions (Claude Code, Codex, Copilot) of the same
generic prompt, each browsable in its own tab.

No build step. No frameworks. Plain HTML/CSS/JS — open `index.html` in a
browser and it works; push it to GitHub Pages and it's live.

## What's in here

```
index.html      page shell (rarely needs editing)
styles.css      design system (rarely needs editing)
app.js          rendering logic (rarely needs editing)
data.js         ALL WORDS AND NUMBERS LIVE HERE — edit this
images/         the paper's screenshots (AI transcripts, program output)
code/*.js       embedded source code shown in the code browsers (generated)
source/         the real .cpp/.hpp/.txt files, one folder per implementation
tools/embed.py  regenerates code/*.js from source/
```

## How to update things

**Change any text, table, output, or fact** → edit `data.js`. It's one big
commented object: the essay is `SITE.blog`, each tab is
`SITE.implementations`, the comparison tables are `SITE.comparison`, and the
hero numbers are `SITE.rail`. Save, refresh, done.

**Add a screenshot** → drop the PNG in `images/`, then paste this block into
the relevant `html:` string in `data.js`, at the point in the essay where it
belongs. `width`/`height` are the image's real pixel size — they reserve the
space so the page doesn't jump while it loads. Clicking any screenshot opens
it full size.

```html
<figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Who / what this is</div>
    <img src="images/FILE.png" width="1476" height="868" loading="lazy" alt="What the screenshot shows, for screen readers.">
  </div>
  <figcaption>The caption printed under it.</figcaption>
</figure>
```

**Change the code shown in a code browser** → replace the file under
`source/<implementation>/`, then run:

```
python3 tools/embed.py
```

That regenerates `code/*.js`. (To add a brand-new file, also add its name to
the `MANIFEST` at the top of `tools/embed.py`.)

**Add a whole new implementation tab** →
1. make `source/<new-id>/` with its files, add it to `MANIFEST` in
   `tools/embed.py`, run the script;
2. add `{ id: "<new-id>", label: "..." }` to `SITE.tabs` in `data.js`;
3. add a matching entry to `SITE.implementations`;
4. add `<script src="code/<new-id>.js"></script>` in `index.html`.

## Publishing on GitHub Pages

One-time setup:

```
cd this-folder
git init
git add .
git commit -m "Roadtrip optimizer comparison site"
git branch -M main
git remote add origin https://github.com/YOUR-USERNAME/roadtrip-optimizer.git
git push -u origin main
```

Then on github.com: repo → **Settings → Pages** → Source: **Deploy from a
branch** → Branch: **main**, folder **/ (root)** → Save.

Your site is live in ~a minute at:

```
https://YOUR-USERNAME.github.io/roadtrip-optimizer/
```

Every update after that is just:

```
git add . && git commit -m "update" && git push
```

GitHub redeploys automatically.

## Previewing locally

Double-clicking `index.html` works in every modern browser (all code is
embedded — no server needed). If you prefer a local server:

```
python3 -m http.server 8000
# then open http://localhost:8000
```

## AI assistance and credits

This website and the project it presents were built with AI assistance, and
that usage is the study's subject matter. Claude (Anthropic's Claude Code)
was the primary contributor to the website build and to the hand-specified
project's code, working from the author's specifications and reviews. The
study presented here cites and analyzes codebases produced from generic
prompts by Codex (ChatGPT), Claude Code, and Microsoft Copilot; those
codebases are shown unmodified in the code browsers.
