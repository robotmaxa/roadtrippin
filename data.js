/* ============================================================
   SITE CONTENT — edit this file to update the website.
   No HTML or CSS knowledge needed for most changes:
   - site meta and hero numbers: SITE.meta / SITE.rail
   - the essay: SITE.blog (array of sections; html allowed)
   - each tab: SITE.implementations (facts, story, output)
   - comparison tables: SITE.comparison
   To update source code shown in the code browsers, replace the
   file under source/<impl>/ and run: python3 tools/embed.py
   ============================================================ */

window.SITE = {

  meta: {
    title: "Making AI Write My Code",
    subtitle: "instead of AI writing my code",
    author: "One project. One generic prompt. Three AIs. Four codebases.",
    tagline:
      "A study of how phrasing alone produces entirely different codebases for the same goal — and why knowing the algorithm before the prompt is what keeps a project on the road.",
    footer:
      "Roadtrip Optimizer — a study in specification vs. prompting. Verified by brute force.",
  },

  /* The signature: the optimal route's strictly decreasing miles-remaining.
     This is the monotone progress invariant, visible in the output. */
  rail: {
    origin: "Denver",
    destination: "San Francisco",
    totalMiles: "1,572.57",
    score: "4.80",
    /* left  = miles still to run to San Francisco (strictly decreasing)
       driven = miles actually travelled so far (cumulative, increasing) */
    originLeft: 1212,
    stops: [
      { name: "Rocky Mountain NP", left: 1142, driven: 69 },
      { name: "Arches NP",         left: 873,  driven: 365 },
      { name: "Grand Canyon NP",   left: 725,  driven: 654 },
      { name: "Bryce Canyon NP",   left: 699,  driven: 787 },
      { name: "Zion NP",           left: 644,  driven: 850 },
      { name: "Sequoia NP",        left: 288,  driven: 1239 },
      { name: "Yosemite NP",       left: 197,  driven: 1376 },
    ],
    destinationDriven: 1573,
    caption:
      "Two axes on every marker: miles still to run, and miles actually driven to get there. mi-left is strictly decreasing down the route — 1212 → 1142 → 873 → 725 → 699 → 644 → 288 → 197 → 0 — while driven climbs 0 → 1,573. That single decreasing invariant makes the graph acyclic, and the acyclicity is what makes the solver polynomial.",
  },

  tabs: [
    { id: "study",       label: "The Study" },
    { id: "original",    label: "My Project" },
    { id: "claude-code", label: "Claude Code" },
    { id: "codex",       label: "Codex" },
    { id: "copilot",     label: "Copilot" },
    { id: "compare",     label: "Head-to-Head" },
  ],

  /* ---------------- THE ESSAY ---------------- */
  blog: [
    {
      heading: null,
      html: `<p class="lede">Over the past several months, I have experimented with building software alongside AI, exploring how different prompts affect the structure, size, and comprehensibility of the generated code. This is an account of one project where that difference was stark enough to measure.</p>`,
    },
    {
      heading: "Abstract",
      html: `<p>After finishing Data Structures and Algorithms, one of the final topics of study was dynamic programming. With substantial classwork catered towards 1D Dynamic Programming, there was little practice/observation of 2D DP. In order to get experience as well as a genuine curiosity for the programming, the scope of this project was geared towards the preliminary stages of a project to find an efficient and optimal solution to finding ideal routes/rankings based on a specific dataset. The final project includes two parts. The first, ROADTRIP, takes a selection of 59 hand-curated locations labeled with location name, latitude, longitude, and an opinionated star rating out of 5, with natural sightseeing locations rated the highest. The data contains cities, towns, and national parks across the western United States, as this project is tailored to finding an ideal road trip from Denver to San Francisco. Every stop has to leave less road remaining to the destination than the last one did, and that single constraint is what makes the rest work: a strict decrease means the graph is acyclic, so the DP never has to track which regions have already been visited. Each leg also has to fall inside a daily driving window, defaulting to 50 to 450 miles, while the final drive into the destination is bounded separately, since a day of driving is a day you plan around and arriving is not. It uses a 2D DP that tracks dp[stops][region], tracking the best way to reach each region in a certain amount of stops while maximizing the mean star rating across exactly the number of nights requested. Once the table is completed, it employs a search across every region within reach of the destination to find the best route, all of which runs in O(nights&nbsp;·&nbsp;n²).</p>
<p>The OPTLOOP (second part) employs 2D dynamic programming to solve the Travelling Salesperson Problem on a small set of data (15 locations), capped at 20 regions because the table grows as 2ⁿ&nbsp;·&nbsp;n and hits a memory wall past that rather than a time one. This involved the Held-Karp algorithm, which took extensive research to understand bit masking and the algorithm as a whole, and runs in O(2ⁿ&nbsp;·&nbsp;n²). The distance between those two complexities is the entire point: a one-way trip has an ordering to exploit and a loop does not. Both solvers are checked against a separate brute-force program that shares no search code, using exhaustive DFS for ROADTRIP and full permutation enumeration for OPTLOOP.</p>
<p>The scope of this entire project is intended to be the preliminary stages of a small subproblem with real capacities for translation to larger datasets. This paper is an investigation into how three AIs (Claude Code, Codex (ChatGPT), and Copilot) react to a prompt that describes this very problem in more generic terms. It is meant as an investigation into holes in prompting and how prompting a small subproblem generically can lead to large holes in complexity as the coding project grows. This is an examination of these small holes and the importance of understanding algorithms before prompts in order to efficiently and correctly attack an intended project. Held-Karp is not included in the scope of this examination, as it was an evolved point in the original project when the scope of the project was decided to be done via dynamic programming.</p>`,
    },
    {
      heading: "The first attempt",
      html: `<p>The idea came before the education. I wanted a program that took specific details about my own preferences (such as rigorous hikes, scenic views, modes of travel) and returned the places that best matched them, not the most popular option but the most suitable one. Nature and sightseeing seemed like the most specific version of the problem, and one I genuinely enjoyed: if I want to go on a walk or spend a day at a lake, how do you turn &ldquo;best place to go&rdquo; into something computable? Topography, review scores, hike difficulty ratings. Eventually I wanted the same engine to work on restaurants, campsites, movies, road trips, anything with enough attributes to rank.</p>
<p>At the time I knew very little about algorithms. I asked Claude Code to build an algorithm for a hiking trail ranking system.</p>
<p>What came back was over a thousand lines of code, most of it machinery for pulling map data from external APIs. The output looked impressive, and my first instinct was to assume it worked, that the code was doing what I had pictured. Logically I knew I couldn't have described the scope of the problem in the handful of rushed, unspecific prompts I had given it. The program appeared to be returning desirable hikes, though I couldn't read it well enough to confirm that. That gap was the frustrating part. I couldn't tell whether I had what I asked for, and after a solid amount of observation, I realized the code output was not giving me any real values for which desirable hikes to try. As it turned out, the code evaluated almost none of what I cared about. It was returning hiking routes filtered by mileage.</p>
<p>A few computer science courses later, I understood the failure differently. The problem wasn't an inability to read code I hadn't written. It was that I had given myself no patterns to recognize, because I had never specified any. The first version ran off in directions I couldn't follow because I never told it where to go.</p>`,
    },
    {
      heading: "Narrowing the problem",
      html: `<p>Some months later I dropped the hiking version entirely. The issue was that the data needed required extensive map APIs and caused overcomplication of a problem that needed only data analysis, not data retrieval. So I narrowed the scope to something I could reason about end to end: a cross-country road trip planner covering several nights, with a hard cap on how far I would drive in a day. It would read everything from text files rather than the internet, which removed the API machinery that had swallowed the first attempt. The motivation was practical. I was about to drive across the country, and I wanted to see whether I could build something that would generate a genuinely good itinerary of places to camp along the way. With a better grasp of algorithms, I also knew the optimization itself didn't require complicated code, and I wanted to find out whether dynamic programming, which I had just finished studying, could solve it in a form short enough to read.</p>`,
    },
    {
      heading: "The specification",
      html: `<p>I wrote the request as a specification. Instead of describing an outcome, I named the algorithms I wanted used: Dynamic Programming over a Directed Acyclic Graph (DAG) because this road trip only progresses in one direction, which makes the graph acyclic and the DP cheap as the linear sort guarantees each subproblem is evaluated exactly once. Then the second part was a Dynamic Programming solution to the traveling salesperson problem for optimal loop routing on shorter trips, rather than a heuristic one.</p>
<p>When I specified my desire to create the project via Dynamic Programming to find the best possible road trip route in C++, it generated a result that was roughly 300 lines on the first attempt. Although it wasn't perfectly aligned with my intentions, I could read all of it. That meant I could immediately identify the sections that weren't doing what I originally planned, the thing the first attempt had made much more difficult. After sharpening the code and studying the Held-Karp algorithm to handle a new TSP side of the problem, I had a working skeleton of the idea I'd had months earlier: a program that traverses arbitrary data points and returns the best results for a specified set of preferences. There is a great deal left to build on top of it, which I plan on adding at some point, but the backbone holds.</p>`,
    },
    {
      heading: "The experiment",
      html: `<p>To test out this code generation with specific and general prompts. Using the carefully coded and prompted creation of the final project and using it as a representative of specific, intentional prompting to compare against the generic prompting listed below on several AIs (Claude Code, Codex, Copilot). The goal became to see how this specific project's time complexity, space complexity, and code auditability compared to a generic prompting of AIs.</p>
<p>Every run attached the same file the project uses: western-usa.txt, 56 regions between Denver and San Francisco at the time of these runs (since grown to 59) for the road trip mode. After prompting, this file was given to the AIs to adjust their formatting to this input file and return their results.</p>
<p>The generic prompt:</p>
<blockquote class="prompt-block">Create a C++ project that finds me 4-7 of the best spots to visit on a road trip that can be 4-7 days (4-7 is the range the constant can be in). depending on the destination, you can only move east or west, never go the opposite way, but you can drive up or down. You cannot exceed 450 miles in one day, and you must return the most ideal trip with the coolest natural places to visit (national parks/ natural novelties are rated higher than cities). the best route possible should be returned with location, stop number, miles to that location from the last, and headed with total miles driven</blockquote>
<p class="aside">(NOTE: In the original project creation, the Held-Karp algorithm was not planned so it was not prompted here, as that specific subproblem gives away that this project should be dynamically programmed. Held-Karp was an evolved state of this project.)</p>`,
    },
    {
      heading: "What came back",
      html: `<p>The answers diverged. Codex proposed depth-first search with backtracking; Claude proposed DFS with branch and bound. Both produced working, optimal solutions, but both are exponential in the worst case. The DAG formulation I'd specified runs in polynomial time, which made it the cheapest of the three.</p>
<div class="table-scroll"><table>
<thead><tr><th></th><th>Codex</th><th>Claude Code</th><th>Copilot</th><th>My project</th></tr></thead>
<tbody>
<tr><td>Search structure</td><td>DFS + backtracking</td><td>DFS + branch and bound</td><td>DFS + backtracking</td><td>DP over (night, region)</td></tr>
<tr><td>Worst-case cost</td><td>O(n² + nᵏ)</td><td>~6 billion paths</td><td>O(k · nᵏ)</td><td>O(nights · n²)</td></tr>
<tr><td>Exact optimum</td><td>No</td><td>Yes</td><td>Yes</td><td>Yes</td></tr>
<tr><td>Road factor</td><td>1.00 (great-circle)</td><td>1.18 (self-chosen)</td><td>1.00 (great-circle)</td><td>1.25 (specified)</td></tr>
<tr><td>Min daily mileage</td><td>none</td><td>50 mi, unprompted</td><td>none</td><td>50 mi, by measured sweep (originally 100)</td></tr>
<tr><td>Considered DP?</td><td>no; justified skipping it</td><td>&ldquo;I never considered it&rdquo;</td><td>no; justified skipping it due to data selection</td><td>chose it deliberately</td></tr>
</tbody></table></div>
<div class="table-scroll"><table>
<thead><tr><th>places (n)</th><th>search worst case P(n,7)</th><th>DP at 7 · n²</th><th>ratio</th></tr></thead>
<tbody>
<tr><td>14</td><td>17,297,280</td><td>1,372</td><td>12,607×</td></tr>
<tr><td>20</td><td>390,700,800</td><td>2,800</td><td>139,536×</td></tr>
<tr><td>40</td><td>93,963,542,400</td><td>11,200</td><td>8,389,602×</td></tr>
<tr><td>59 (my dataset size)</td><td>1,719,393,207,840</td><td>24,367</td><td>70,562,367×</td></tr>
</tbody></table></div>
<p class="table-caption">Worst-case route-search operations at 7 stops. DFS with backtracking and DFS with branch and bound share the same P(n,7) worst case, since pruning is a practical speedup rather than a guarantee; the DP runs at 7 times n squared.</p>
<p>My original prompt never specified an input format, and both models filled that gap with a data layer I didn't ask for. An unspecified input format does not produce less code. It produces more.</p>
<p>Three smaller observations from the outputs. First, Codex used raw great-circle distance with no road factor at all, so its reported 1,118-mile trial total quietly understates real driving on every leg. Second, both models invented a mileage penalty in their objective without being asked and picked different weights for it: Codex scored route_value = score − 0.015 × miles, Claude Code used 0.0015 × miles, a tenfold difference in how much a mile costs, each constant hand-picked and unexplained. A convergent structure with arbitrary numbers. These arbitrary calculations reduced the soundness of the actual calculations, although the routes returned were high scoring routes. Third, the minimum-mileage problem showed up in both outputs from opposite directions: Claude Code noticed it and patched it unprompted with a 50-mile floor. Codex never noticed: its own best route contains a 32-mile &ldquo;day&rdquo; from Arches to Canyonlands. I had originally hit the same flaw and set a 100-mile floor by design; later, after sweeping the floor against attainable score, I moved it to 50, the same number Claude Code had picked unprompted.</p>
<p>That was the satisfying part: I had shaped an algorithm that was correct, efficient, and assembled from patterns I already knew. Not to mention, different AI's implementations had shown real places where the final project's code could be improved. This success led to a question that prompted my work on this project: Why does phrasing alone produce entirely different codebases for the same underlying goal?</p>`,
    },
    {
      heading: "Why none of the answers were wrong",
      html: `<p>What is to be learned is that it doesn't come down to the AI's strengths at this point, it comes down specifically to the holes that a prompt may have. The challenge, however, was that none of the AI outputs to the question were incorrect. They all provided the optimal answer to the question in their own datasets (however, scoring for the final project's data was more optimal than Codex, but that is to be expected based on the specified requirements I had made). The generic prompt reads as a textbook TSP problem via unordered search, and branch and bound and DFS with backtracking are textbook solutions to that problem. So was it that my prompt was too generic?</p>
<p>Fascinatingly enough, that is actually not the full story. My error in running this trial of comparison is that I did not consider the fact that even my generic prompt was just a slightly dumbed down version of an actual explanation of my DAG DP problem. There is literally only a single phrase in the entire prompt that the AI interpreted differently that kept it from solving via DP: &ldquo;you can only move east or west, never go the opposite way, but you can drive up or down.&rdquo; If you change that sentence to &ldquo;Every stop has to bring me closer to the destination than the last one did,&rdquo; you get the AI returning a DAG DP solution.</p>
<p>For the original prompt, it contained the same rule: you can't go backwards. The generic prompt states it as a prohibition, a move that isn't allowed, and a prohibition reads as a filter you apply while searching. That is exactly what branch and bound and DFS with backtracking are. Changing that one sentence shifted the narrative from prohibition to an ordering, changing the problem entirely.</p>
<p>What this means is that in this small sample, we see a situation where I, the user, was mere words away from solving with a different, less efficient algorithm. This highlights the risk of prompting, and how a knowledge of what code you plan to use can keep your final product from deviating into different algorithms and methods. In this case, our code only ended up less efficient, but in larger products this could result in complete differentiation in functionality. This sheds light on the importance of knowing where your program is supposed to be going, so you can quickly readjust your code when AI misinterprets a few words in your prompt. Asking your agent what algorithms it plans on employing before the code gets written can help catch deviations earlier.</p>`,
    },
    {
      heading: "What it costs when you can't read the answer",
      html: `<p>So how can I make one good prompt without having to spend hours arguing with AI over what I really want the project to look like in my mind? Well, it seems that I must know exactly how to code the problem in order to direct AI properly. The end product of my refined project was not difficult to explain on paper. AI served as a tool to collapse execution time, but not thinking time. Every word had to come as a calculation, and my education gave me the foundation to invest that time in critical thinking. When I asked Claude Code (generic prompting) why it did not use DP optimization, it returned that it did not even consider it. It identified the problem it wanted to solve, and once it realized it worked, it stopped searching for better solutions. Its own words:</p>
<blockquote>&ldquo;I didn't weigh DP and reject it. I never considered it. The README documents the branch-and-bound thoroughly across seven design sections and doesn't mention dynamic programming once. That absence is the tell. It wasn't a trade-off, it was a reflex.&rdquo;</blockquote>
<p>Pressed further, it found the evidence had been sitting in its own design note the whole time. Its bounding function, a suffix table built back-to-front by tabulation, is itself a Dynamic Program. It wrote a DP to prune a search over a problem that was already a DP. And it named the one-minute audit it had skipped: go through each constraint and ask whether it is path-cumulative or edge-local. Every constraint in my problem is edge-local, so the state collapses to (current place, stops used), and the whole thing is a DP. By Claude's own account, it did not arrive there independently.</p>
<figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Claude Code &middot; follow-up run</div>
    <img src="images/claude-code-why-not-dp.png" width="1250" height="832" loading="lazy" alt="Claude Code transcript. Asked to state just the algorithm used, it answers DFS with branch and bound. Asked why it did not use dynamic programming via DAG, it answers that there is no principled reason, that it never considered it, that it had already done the DAG reduction and ran an exhaustive search over the DAG anyway, and that the state would have been (node, stops used).">
  </div>
  <figcaption>Claude Code's response to why it did not utilize dynamic programming.</figcaption>
</figure>
<p>Pressed on the same question in a follow-up run, it went further: &ldquo;There's no principled reason. I didn't evaluate it and reject it — I never considered it.&rdquo; It had already done the DAG reduction itself and written it down as the key move, then ran an exhaustive search over the DAG anyway, &ldquo;the one thing the reduction exists to make unnecessary.&rdquo; It also diagnosed why the reflex fired: the exactly-K-stops requirement made the problem read as not quite the textbook DAG longest-path, and the standard fix, adding stop count as a second state dimension, dp[k][v] over a layered DAG, is precisely the state my project uses.</p>
<figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Codex &middot; ChatGPT</div>
    <img src="images/codex-why-not-dp.png" width="926" height="1269" loading="lazy" alt="Codex transcript. Asked why it did not use dynamic programming, it answers that the problem was small enough that DFS with backtracking and pruning was simpler and still fast, and lists the state DP would need: current location, number of stops chosen, which places have been visited, direction constraints, and score versus mileage tradeoff.">
  </div>
  <figcaption>Codex's response to why it did not utilize dynamic programming.</figcaption>
</figure>
<p>Codex, asked the same question, produced a justification rather than a concession. DP, it said, would be more complex because the state would need to track the current location, the number of stops chosen, which places have been visited, the direction constraint, and the score-versus-mileage tradeoff. It stated that the smaller data did not need more than DFS directly. Three of those five do not belong in the state at all. The one-direction rule is exactly what removes the visited set, the direction constraint folds into the index ordering, and an additive mileage penalty folds into the edge weight. Its explanation for skipping DP rested on state the problem never needed.</p>
<figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Microsoft Copilot</div>
    <img src="images/copilot-why-not-dp.png" width="756" height="1114" loading="lazy" alt="Copilot transcript. Asked why it did not use dynamic programming, it lists five reasons under the heading Why DFS instead of DP: direction lock, variable trip length, scoring system, constraint pruning, and search space size.">
  </div>
  <figcaption>CoPilot's response to why it did not utilize dynamic programming.</figcaption>
</figure>
<p>CoPilot returned the most concise explanation of why it did not use Dynamic Programming at all, highlighting the scope of the input as the leading factor in how it decided which program to run, as well as directional constraints.</p>
<p>If unaware of the algorithms that could solve the problem, the user would never be able to orient the AI toward the answer they actually wanted exactly. And if it returned code the user couldn't read, the user would struggle to explain that isn't what they're after and how to fix it. The problem is right there in how stops weren't being ordered.</p>
<p>This happened in the first working version of the final project, the program decided whether one stop could follow another by comparing how far each sat from Denver. Farther from the start counted as progress. That is not the same thing as getting closer to where you're going, and the difference showed up as routes detouring hundreds of miles north before turning back west. One went Arches, then Jackson Hole, then Bryce Canyon, each stop farther from Denver than the last, while the middle one left me 276 miles farther from the destination of that version, Joshua Tree, than the stop before it. Nothing in the code was broken. It compared the wrong two numbers, and it compared them because that is what I had asked for. My own constraint said never move back east, and every one of those stops was west of the last. Finding it meant understanding that the ordering was the thing making the graph acyclic, and that changing which point it measured from would keep that property while fixing the behavior. It was not AI's shortcoming here, it was the gaps in my specifications. And this section alone helped me grasp a much better understanding of this.</p>
<div class="table-scroll"><table>
<thead><tr><th></th><th>Arches</th><th>Jackson Hole</th><th>Bryce Canyon</th><th>verdict</th></tr></thead>
<tbody>
<tr><td>miles from Denver</td><td>320</td><td>493</td><td>519</td><td>all increasing: legal</td></tr>
<tr><td>longitude</td><td>−109.59</td><td>−110.76</td><td>−112.19</td><td>all westward: legal</td></tr>
<tr><td>miles remaining</td><td>613</td><td>889</td><td>415</td><td>Jackson Hole is backward</td></tr>
</tbody></table></div>
<p>Neither the original constraint (east or west only) nor the implementation's axis (distance from origin) catches the detour. Only remaining distance to the destination does, and that became the fix: the ordering axis flipped from distance-from-origin, increasing, to distance-to-destination, strictly decreasing. Same acyclicity, correct meaning of progress.</p>`,
    },
    {
      heading: "Why Held-Karp?",
      html: `<p>The same project contains the opposite case, and keeping it was deliberate. The loop mode has no direction of progress. A trip that starts and ends at home has no ordering to exploit, so the places already visited have to live in the algorithm's state, and the right tool is Held-Karp: dynamic programming over subsets with a bitmask. One project, two subproblems, and the structure that makes the corridor cheap simply does not exist in the loop. That contrast is the clearest evidence that choosing DP for the corridor was about the problem's shape, not about DP being a magic word. That was the progress of working through this project and identifying the utility in Dynamic Programming myself, thus why Held-Karp was not included in the general prompt.</p>
<div class="table-scroll"><table>
<thead><tr><th></th><th>ROADTRIP — corridor mode</th><th>OPTLOOP — loop mode</th></tr></thead>
<tbody>
<tr><td>time</td><td>O(nights · n²)</td><td>O(2ⁿ · n²)</td></tr>
<tr><td>space</td><td>O(nights · n)</td><td>O(2ⁿ · n)</td></tr>
<tr><td>why</td><td>progress order replaces a visited set</td><td>no ordering — the state must remember visits</td></tr>
<tr><td>in practice</td><td>59 regions, instant</td><td>capped at 20 regions (0.25 GB; n=22 needs 1.1 GB)</td></tr>
</tbody></table></div>
<p class="table-caption">One project, two subproblems, two complexity classes. The structure that makes one mode polynomial does not exist in the other.</p>`,
    },
    {
      heading: "What no longer remains in the final project",
      html: `<p>Two of the more sophisticated pieces of this project no longer exist and removing them was part of its progress. The original project built length normalization so trips of different lengths could compete: pad the shorter route with average-quality phantom stops, divide by the maximum. Padding with a route's own average provably cancels back to a plain average and is why a global average avoids that paradigm. Then, the number of nights were made an exact input, every route had the same length, and there was nothing left to normalize. For the sake of refraining from slightly arbitrary and unnecessary math for the sake of the problem, this calculation was removed. The ranked-alternatives table went the same way. Keeping clever code not entirely necessary would have been the same mistake as the hiking version: more minute arbitrary data to track which is not highly justifiable and just confuses the code space of the algorithms I created. It made it harder to track if my function was working properly in general, or just with the specific tests and datasets I was using.</p>
<pre class="before-after"><code>before:  finalScore = (rawSum + (MAX_STOPS - stops) * globalAvg) / MAX_STOPS

after:   finalScore = rawSum / nights</code></pre>`,
    },
    {
      heading: "Proving it right",
      html: `<p>The generic prompt's algorithms did not go to waste. Exhaustive search is a terrible way to solve this problem and a perfect way to check it, so the naive approach became the verifier: a second program that finds the answer by brute force and gets diffed against the solver on hundreds of random inputs. The two programs share the parsing and the distance math out of necessity, but nothing about how they search.</p>
<p>The diff testing caught exactly one real disagreement, and it was worth the whole harness. On one dataset the solver and the checker returned identically scored routes through different stops: the solver went through Grand Canyon, the checker through Zion, both rated the same. Neither was wrong. The solver breaks ties between equal states toward the larger node, which cascades into preferring the route that is lexicographically larger, and my checker had no opinion on ties at all. The mismatch existed because a rule was implicit in one program and absent in the other. The fix was stating the tie-break policy explicitly in both, and the lesson is that a verifier does not just catch wrong answers, it forces every unstated decision out into the open.</p>
<p>Re-run against the final architecture at the split defaults: 1400 of 1400 random ROADTRIP instances match the exhaustive DFS on full output, and 200 of 200 OPTLOOP instances match the permutation check.</p>`,
    },
    {
      heading: "Known Limitations",
      html: `<p>Straight-line distance times 1.25 misses geography that forces detours, so some legs are optimistic. This is OK for the scope of this project, as the main center of focus was the algorithms efficiency as opposed to validity of routes. Something to factor in future renditions. The star ratings are my own judgment, not sourced data. The datasets are small enough that long trips exhaust them. And the random generator scatters regions across a box rather than along the Denver-to-San-Francisco corridor, which is why 7 of 25 generated instances need daily legs over 450 miles and stay unsolvable at the new defaults. Each of these is a known simplification, not a surprise. The scope is to only use data that works for the program, but AI had filled in the gaps on the situations where routes cannot complete.</p>`,
    },
    {
      heading: "Results",
      html: `<p class="aside">DISCLAIMER: During the running of the actual one-run outputs of the generic prompting, I did nothing to add to the prompt to adjust it. The difficulty of making adjustments is not factored in here, and instead my polished project is simply compared to the direct results from the other projects adjusted to using my western-usa.txt file. The trip14.txt file was not used by the other AIs, neither was OPTLOOP. I mention these below in my analysis of my polished function to further explain edits I made to my code and input files to correctly process my intended outputs.</p>
<p>Exact output from the final project ROADTRIP mode on western-usa.txt (59 regions, defaults: 7 nights, 50–450 mi daily, 700 mi final approach): Score 4.80 (mean site rating), 1,572.57 total miles including the 196.71-mile final drive: the optimum under the 450 ceiling. Every stop is a National Park.</p>
<figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">My project &middot; bestRoute on western-usa.txt</div>
    <img src="images/original-bestroute-output.png" width="1476" height="868" loading="lazy" alt="Roadtrip mode output on the western dataset. Path Denver, Rocky Mountain NP, Arches NP, Grand Canyon NP, Bryce Canyon NP, Zion NP, Sequoia NP, Yosemite NP, San Francisco. Total 1572.57 miles, mean site score 4.80, with a mi-left-to-SF column reading 1211.66, 1142.24, 873.15, 724.80, 699.21, 644.34, 287.71, 196.71, 0.">
  </div>
  <figcaption>The mi-left column is strictly decreasing down the list, 1142 → 873 → 724 → 699 → 644 → 287 → 196: the monotone progress invariant visible right in the output.</figcaption>
</figure>
<p>The floor stays at 50, a measured choice from sweeping it against attainable score: the score is weakly decreasing in the floor, since a lower floor only ever adds edges, and the 62.92-mile Zion leg is what 50 buys. The ceiling took more work, because dropping it to 450 initially broke trip14 entirely — and the diagnosis was that one flag was doing two unrelated jobs: capping a day's drive and capping the final approach into San Francisco. trip14.txt, as the file then stood, had zero regions within 450 miles of SF, so the finish test starved. The fix was to split them, which is what the minimum was already doing, since the final leg never had a floor. The flags are now --lo 50 for the shortest daily drive, --hi 450 for the longest, and --final 700 for the final approach, which also caps the loop mode's leg home. I later added Yosemite to trip14 itself, which puts a region 197 miles from the destination and removes the starvation from the data side as well. A 4.81-scoring Yellowstone route needs a daily ceiling of 550 or more for its 540-mile opening leg, so this 4.80 route is the true optimum at 450: 0.01 of score given up for 672 fewer miles, and no day exceeds 389. This is a quintessential point in the project, because this data adding goes to show that the selective data used is equally as important as the actual code. Without Yosemite and data points that allow for shorter drives in OPTLOOP, the program fails and the code to fix it becomes altogether much more complicated. This was a learning curve in AI usage: how to keep AI from solving every single problem when the problem should be much smaller. That fix being to use carefully curated data. Yosemite (4.9) and Sequoia (4.7), the two closest regions to San Francisco in the file, fill the final legs and let the trip finish 196 miles out instead of 644. Against the original 56-region file, the score went 4.71 → 4.80 and total mileage 2,442 → 1,573. Making the final product less exhaustive yet just as optimal.</p>
<div class="table-scroll"><table>
<thead><tr><th></th><th>single --hi 450</th><th>split 450 / 700</th></tr></thead>
<tbody>
<tr><td>western-usa ROADTRIP</td><td>4.80</td><td>4.80</td></tr>
<tr><td>trip14 OPTLOOP</td><td>2732.12</td><td>2732.12</td></tr>
</tbody></table></div>
<p>To make the numbers comparable, all models were rerun against western-usa.txt and the 0–5 rating scale. Claude Code, adapted to western-usa.txt with the rating used directly returned a 7-stop route at a 4.80 mean rating and 1299 miles, longest day 367, shortest 59. One caveat, Claude Code's road factor is 1.18 against my 1.25, so even with the window now matched at a 450 cap and 50 floor, the edge sets are not quite identical.</p>
<figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Claude Code &middot; rerun on western-usa.txt</div>
    <img src="images/claude-code-route-rerun.png" width="1486" height="766" loading="lazy" alt="Claude Code output: 7 days, 7 stops, 1299 miles, mean rating 4.80, longest day 367 miles against a 450 cap. Stops are Rocky Mountain NP, Arches NP, Grand Canyon NP, Bryce Canyon NP, Zion NP, Sequoia NP, and Yosemite NP.">
  </div>
  <figcaption>Claude Code rerun against western-usa.txt on the 0–5 scale: the same seven parks, 1,299 miles, 4.80 mean.</figcaption>
</figure>
<p>Codex's scoring logic on the same dataset produced a different character of route. As seen below, it selects a series of much shorter routes, travelling 32 miles, 70 miles, 77 miles, and then 50 miles to a few lower caliber scoring areas, leading to an average location scoring of 4.69. This happened because of there being no lower mileage cap and Codex generating a low mileage incentive.</p>
<figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Codex &middot; rerun on western-usa.txt</div>
    <img src="images/codex-route-rerun.png" width="1480" height="848" loading="lazy" alt="Codex output: Denver to San Francisco, 1013.9 miles, place score 32.8 of 35, route value 17.592. Stops are Black Canyon NP, Arches NP, Canyonlands NP at 32 miles from the last stop, Capitol Reef NP at 70, Bryce Canyon NP at 77, Zion NP at 50, and Yosemite NP at 359.">
  </div>
  <figcaption>Codex's route on the same dataset — the short 32, 70, 77 and 50-mile legs are the days its objective was willing to spend on lower-rated stops.</figcaption>
</figure>
<p>Microsoft CoPilot used depth-limited exhaustive DFS with backtracking and feasibility pruning, resulting in the optimal solution in a single main.cpp. Although it holds no low mileage restriction, it does not incentivize low mileage, allowing for an optimal route path.</p>
<figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Microsoft Copilot &middot; rerun on western-usa.txt</div>
    <img src="images/copilot-route-rerun.png" width="1482" height="764" loading="lazy" alt="Copilot output: best westbound route from Denver, 1100.69 miles, score 332.331. Stops are Rocky Mountain NP, Arches NP, Grand Canyon NP, Bryce Canyon NP, Zion NP, Sequoia NP, and Yosemite NP.">
  </div>
  <figcaption>CoPilot's best westbound route from Denver: the same seven parks again, 1,100.69 miles, out of 180 lines of a single main.cpp.</figcaption>
</figure>
<p>Proof of functionality sits in the brute force route findings of the 59-region file. The final project's own checker, and these AIs DFS confirming the optimal route of the same 4.80 mean with the same five-park spine: Rocky Mountain, Arches, Grand Canyon, Bryce, and Zion. One more convergence the unified tables make visible: every AIs route in this section, from every planner, is strictly decreasing in miles remaining to San Francisco and none of them was told that invariant, and none of them violates it.</p>`,
    },
    {
      heading: "Takeaways",
      html: `<p>It seems that the user must know exactly how to code the problem in order to direct AI properly. AI served as a tool to collapse execution time, but not thinking time. The education is what bought the thinking, and the code that came back was only useful because I could read it, doubt it, and catch the one place where my own specification was wrong.</p>
<p>One test from this experiment is worth carrying forward: for every constraint in a problem like this, ask whether it is path-cumulative or edge-local. If everything is edge-local, the state collapses and the problem is a Dynamic Program. If the values accumulate along the whole path, it isn't Dynamic. That audit takes a minute, and it is the difference between the two complexity classes this entire experiment kept producing.</p>
<p>In this experiment, small wording changes consistently led different models toward different algorithmic formulations. In future projects, I do not believe I can simply avoid accidental phrases the AI interprets as a different type of project. Going into coding, I must refine the exact algorithms I plan to utilize so that AI can better understand the objectives I am trying to accomplish. My own project contains its own set of limitations and is still in progress. These edits are not ideal for AI, as any suggestion made runs the risk of altering or adding code to increase risks of manifested additions of code. This has been a study of the limitations of code writing for any AI usage, including in my own project. What I do know, is that my algorithm optimally and efficiently solved the exact specification at hand: a road trip destination planner given a specific set of input.</p>
<p>Each of these AIs were able to recognize how to effectively use my algorithms and reinforced how such algorithms gain relevance as the dataset grows. The biggest lesson wasn't that AI produced inefficient code. It was that AI faithfully optimized the problem I described, not the one I imagined. Once I understood the algorithm myself, prompting stopped feeling like guesswork and became another programming language.</p>
<p>As a final comparison, here is what Google search's AI returns me when I simply ask it to return to me a 7-day road trip to San Francisco from Denver:</p>
<figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Google search AI</div>
    <img src="images/google-ai-prompt.png" width="920" height="190" loading="lazy" alt="The prompt given to Google search's AI: give me the best 7 stop roadtrip that finishes in san francisco on day 8 when travelling from Denver to San Francisco. nature preferred.">
    <img src="images/google-ai-itinerary.png" width="801" height="767" loading="lazy" alt="Google search AI's answer: an eight-day table of destination stops with driving distance and remaining distance to San Francisco — Grand Junction CO, Moab UT, Torrey UT, Bryce Canyon UT, Baker NV, South Lake Tahoe CA, Yosemite National Park CA, and San Francisco CA.">
  </div>
  <figcaption>The same question put to Google search's AI, and the itinerary it returns.</figcaption>
</figure>
<p>Some similarities, some differences. Still not specific enough data on where to spend the night, a motivating factor in including more specific data in my code.</p>`,
    },
  ],

  /* ---------------- IMPLEMENTATION TABS ---------------- */
  implementations: {

    "original": {
      name: "My Project",
      attribution: "Specified first — the control group",
      signColor: "sign",
      summary:
        "Two solvers behind one CLI. ROADTRIP picks exactly N overnight stops that maximize mean site rating with a DP over (stops, region) on a DAG; OPTLOOP is Held-Karp exact TSP. Everything is derived from geometry, and both solvers are verified against a brute-force checker that shares no search code.",
      facts: [
        { k: "Search structure", v: "DP over (night, region) · Held-Karp for loops" },
        { k: "Worst case", v: "O(nights · n²) corridor · O(2ⁿ · n²) loop" },
        { k: "Lines of code", v: "1,460 across 4 programs (solver 591)" },
        { k: "Verification", v: "1400/1400 ROADTRIP · 200/200 OPTLOOP · 22 fixtures" },
        { k: "Objective", v: "mean site rating over exactly N stops" },
        { k: "Road factor", v: "1.25 × haversine (specified)" },
        { k: "Daily window", v: "--lo 50 · --hi 450 · --final 700, all flags" },
        { k: "Data handling", v: "reads any dataset from stdin; nothing hardcoded" },
      ],
      story: [
        "The load-bearing rule is the progress axis: every leg must strictly decrease mileMark, the road remaining to the destination. Strict decrease makes the graph acyclic, acyclicity removes the visited-set, and that is the entire reason the corridor solver is polynomial while a tour is not.",
        "The naive algorithms the generic prompt produced did not go to waste here — exhaustive search is a terrible way to solve this problem and a perfect way to check it. checker.cpp re-solves every instance with no memoization; tests.cpp diffs the two programs across 1,400 random instances and 22 hand-built fixtures, each isolating one rule.",
      ],
      buildCmd: "make && make test\n./roadtrip --mode ROADTRIP --nights 7 --lo 50 --hi 450 --final 700 < western-usa.txt",
      outputs: [
        {
          title: "ROADTRIP · western-usa.txt (59 regions, defaults)",
          text: "4.80\n0 4 26 44 45 48 58 59\n  stop 1: RockyMountainNP (node 4) with 1142.24 mi left (+69.42 mi drive), site score 4.70\n  stop 2: ArchesNP (node 26) with 873.15 mi left (+295.11 mi drive), site score 4.70\n  stop 3: GrandCanyonNP (node 44) with 724.80 mi left (+289.94 mi drive), site score 4.90\n  stop 4: BryceCanyonNP (node 45) with 699.21 mi left (+132.92 mi drive), site score 4.80\n  stop 5: ZionNP (node 48) with 644.34 mi left (+62.92 mi drive), site score 4.90\n  stop 6: SequoiaNP (node 58) with 287.71 mi left (+388.94 mi drive), site score 4.70\n  stop 7: YosemiteNP (node 59) with 196.71 mi left (+136.59 mi drive), site score 4.90\n  destination: +196.71 mi\n  total: 1572.57 mi",
        },
        {
          title: "ROADTRIP · trip14.txt (15 regions)",
          text: "4.79\n0 3 7 9 11 12 13 15\n  stop 1: RockyMountainNP (node 3) with 1141.95 mi left (+69.64 mi drive), site score 4.70\n  stop 2: ArchesNP (node 7) with 873.15 mi left (+294.86 mi drive), site score 4.70\n  stop 3: MonumentValley (node 9) with 841.74 mi left (+155.75 mi drive), site score 4.60\n  stop 4: GrandCanyonNP (node 11) with 705.70 mi left (+162.91 mi drive), site score 4.90\n  stop 5: BryceCanyonNP (node 12) with 699.21 mi left (+115.04 mi drive), site score 4.80\n  stop 6: ZionNP (node 13) with 644.34 mi left (+62.92 mi drive), site score 4.90\n  stop 7: YosemiteNP (node 15) with 196.71 mi left (+448.30 mi drive), site score 4.90\n  destination: +196.71 mi\n  total: 1506.15 mi",
        },
        {
          title: "OPTLOOP · trip14.txt — Held-Karp exact tour",
          text: "2732.12\n  start: origin (node 0)\n  stop 1: Breckenridge,CO (node 4) with 1112.94 mi left (+73.68 mi drive), site score 3.80\n  stop 2: RockyMountainNP (node 3) with 1141.95 mi left (+78.05 mi drive), site score 4.70\n  stop 3: Vail,CO (node 5) with 1091.82 mi left (+75.81 mi drive), site score 3.80\n  stop 4: ArchesNP (node 7) with 873.15 mi left (+229.23 mi drive), site score 4.70\n  stop 5: BryceCanyonNP (node 12) with 699.21 mi left (+201.83 mi drive), site score 4.80\n  stop 6: ZionNP (node 13) with 644.34 mi left (+62.92 mi drive), site score 4.90\n  stop 7: YosemiteNP (node 15) with 196.71 mi left (+448.30 mi drive), site score 4.90\n  stop 8: JoshuaTreeNP (node 14) with 567.12 mi left (+428.47 mi drive), site score 4.50\n  stop 9: GrandCanyonNP (node 11) with 705.70 mi left (+324.88 mi drive), site score 4.90\n  stop 10: HorseshoeBend (node 10) with 752.75 mi left (+78.91 mi drive), site score 4.40\n  stop 11: MonumentValley (node 9) with 841.74 mi left (+92.03 mi drive), site score 4.60\n  stop 12: CanyonlandsNP (node 8) with 853.61 mi left (+117.23 mi drive), site score 4.60\n  stop 13: MesaVerdeNP (node 6) with 956.60 mi left (+135.31 mi drive), site score 4.50\n  stop 14: GreatSandDunesNP (node 2) with 1146.91 mi left (+202.32 mi drive), site score 4.40\n  stop 15: ColoradoSprings,CO (node 1) with 1194.35 mi left (+104.15 mi drive), site score 3.00\n  return: origin (node 0), +79.00 mi",
        },
      ],
      outputNote:
        "The mi-left column decreases strictly down the ROADTRIP lists — the monotone progress invariant, visible in the output.",
    },

    "claude-code": {
      name: "Claude Code",
      attribution: "AI rendition · generic prompt",
      signColor: "sign",
      summary:
        "DFS with branch and bound over a longitude-sorted DAG, pruned by an admissible suffix top-K bound. Modular OOP across Place/Dataset/Planner, an 847-assertion test suite, and a mileage penalty derived from the rating scale rather than hand-picked. Its bounding table is itself a dynamic program — a DP written to prune a search over a problem that was already a DP.",
      facts: [
        { k: "Search structure", v: "DFS + branch and bound, suffix top-K bound" },
        { k: "Worst case", v: "~6 billion paths (P(n,7)); ~5 ms in practice" },
        { k: "Lines of code", v: "709 + tests" },
        { k: "Own tests", v: "847 assertions, every rule re-verified externally" },
        { k: "Objective", v: "Σ rating − 1e-6 × miles (lexicographic tiebreak)" },
        { k: "Road factor", v: "1.18 × haversine, self-chosen" },
        { k: "Min daily mileage", v: "50 mi floor — added unprompted" },
        { k: "Considered DP?", v: "\u201cI never considered it\u201d — its own words" },
      ],
      story: [
        "It noticed the minimum-mileage problem on its own after stacking Horseshoe Bend and Antelope Canyon, 8 miles apart, as two separate days — and patched it with the same 50-mile floor the hand-built project later reached by measured sweep.",
        "Asked afterward why it skipped dynamic programming: \u201cI didn't weigh DP and reject it. I never considered it. The README documents the branch-and-bound thoroughly across seven design sections and doesn't mention dynamic programming once. That absence is the tell. It wasn't a trade-off, it was a reflex.\u201d Its own comparison of the two approaches: roughly 6 billion worst-case paths against roughly 110,000 DP relaxations — a factor of 54,000.",
      ],
      buildCmd: "g++ -std=c++17 -O2 -o roadtrip main.cpp Place.cpp Dataset.cpp Planner.cpp\n./roadtrip data/western-usa.txt",
      outputs: [
        {
          title: "Adapted to western-usa.txt (rating used directly)",
          text: "==============================================================================\n  TOTAL MILES DRIVEN: 1299\n==============================================================================\n  Start      : Origin\n  Heading    : West only (never reversing)\n  Trip length: 7 days / 7 stops\n  Longest day: 367 mi (cap 450)\n  Trip rating: 4.80 / 5 average over 7 stops\n------------------------------------------------------------------------------\n  STOP  LOCATION                                       RATING      MILES\n------------------------------------------------------------------------------\n  0     Origin                                              -          -\n  1     RockyMountainNP                                  4.70         66\n  2     ArchesNP                                         4.70        279\n  3     GrandCanyonNP                                    4.90        274\n  4     BryceCanyonNP                                    4.80        125\n  5     ZionNP                                           4.90         59\n  6     SequoiaNP                                        4.70        367\n  7     YosemiteNP                                       4.90        129\n------------------------------------------------------------------------------\n  TOTAL                                                             1299\n==============================================================================\n\n  Best trip at each legal length (heading West):\n    4 days:  1116 mi, mean rating  4.85\n    5 days:  1184 mi, mean rating  4.84\n    6 days:  1226 mi, mean rating  4.82\n    7 days:  1299 mi, mean rating  4.80   <-- chosen",
        },
        {
          title: "Its own test suite",
          text: "847/847 checks passed\nAll checks passed.",
        },
      ],
      outputNote:
        "Same seven parks as the hand-built optimum, same 4.80 mean. Its 1,299 miles vs. my 1,573 is the road factor talking — 1.18 vs. 1.25 — not a different route.",
    },

    "codex": {
      name: "Codex",
      attribution: "AI rendition · generic prompt",
      signColor: "sign",
      summary:
        "DFS with backtracking and an optimistic suffix bound, organized in a roadtrip namespace with its own test file. It reads the dataset from disk and enforces the one-heading rule as a filter during search. No road factor and no minimum daily mileage — its best route contains a 32-mile \u201cday\u201d from Arches to Canyonlands.",
      facts: [
        { k: "Search structure", v: "DFS + backtracking, optimistic rating bound" },
        { k: "Worst case", v: "O(n² + nᵏ)" },
        { k: "Lines of code", v: "~315 + ~180 tests" },
        { k: "Own tests", v: "33–43 assertions across revisions" },
        { k: "Objective", v: "rating_sum − 0.015 × miles" },
        { k: "Road factor", v: "1.00 — raw great-circle distance" },
        { k: "Min daily mileage", v: "none — never noticed the problem" },
        { k: "Considered DP?", v: "no; justified skipping it with state the problem never needed" },
      ],
      story: [
        "Asked why it skipped DP, it produced a justification rather than a concession: the state would need the current location, stops chosen, visited set, direction, and the score-versus-mileage tradeoff. Three of those five do not belong in the state at all — the one-direction rule is exactly what removes the visited set, direction folds into the index ordering, and the additive penalty folds into the edge weight.",
        "With no road factor, its reported totals quietly understate real driving on every leg; with no floor, four of its legs are day-trips shorter than 80 miles. The route is optimal under its own objective — that objective just prices a mile at 0.015 rating points, a constant it picked and never explained.",
      ],
      buildCmd: "g++ -std=c++17 -O2 -o roadtrip_planner main.cpp planner.cpp\n./roadtrip_planner data/western-usa.txt 7",
      outputs: [
        {
          title: "Adapted to western-usa.txt · 7 visit stops",
          text: "Best Natural Roadtrip Route - Total Miles Driven: 1014\n\nStart: Denver,CO\nStop 1: BlackCanyonNP | rating 4.4/5 | 168 miles from last\nStop 2: ArchesNP | rating 4.7/5 | 100 miles from last\nStop 3: CanyonlandsNP | rating 4.6/5 | 32 miles from last\nStop 4: CapitolReefNP | rating 4.5/5 | 70 miles from last\nStop 5: BryceCanyonNP | rating 4.8/5 | 77 miles from last\nStop 6: ZionNP | rating 4.9/5 | 50 miles from last\nStop 7: YosemiteNP | rating 4.9/5 | 359 miles from last\nStop 8: SanFrancisco,CA | destination | 157 miles from last\n\nRaw place score: 32.8/35\nRoute value: 17.592\nTotal miles exact: 1013.9",
        },
        {
          title: "Its own test suite",
          text: "All 33 tests passed.",
        },
      ],
      outputNote:
        "Mean stop rating 4.69 on the 0–5 scale — the mileage penalty bought shorter days at the cost of stop quality. Note stop 3: a 32-mile \u201cday.\u201d Measured on the mi-left axis it never violates monotone progress, an invariant nobody told it about.",
    },

    "copilot": {
      name: "Copilot",
      attribution: "AI rendition · generic prompt",
      signColor: "sign",
      summary:
        "The most concise answer: one main.cpp, ~180 lines, depth-limited exhaustive DFS with backtracking and feasibility pruning. The dataset is hardcoded as an initializer list, the heading rule is a longitude check, and there is no road factor and no scoring subtlety — ratingSum × 10 minus miles ÷ 300.",
      facts: [
        { k: "Search structure", v: "depth-limited exhaustive DFS + backtracking" },
        { k: "Worst case", v: "P(n,7) — same exponential class as the others" },
        { k: "Lines of code", v: "~180, single file" },
        { k: "Own tests", v: "none" },
        { k: "Objective", v: "ratingSum × 10.0 − totalMiles / 300.0" },
        { k: "Road factor", v: "1.00 — raw great-circle distance" },
        { k: "Min daily mileage", v: "none" },
        { k: "Considered DP?", v: "no — cited input scope and directional constraints" },
      ],
      story: [
        "Copilot returned the most concise explanation of why it did not use dynamic programming at all, highlighting the scope of the input as the leading factor in how it decided which program to run, as well as directional constraints.",
        "Its search still lands on the same seven national parks as the hand-built optimum and Claude Code's run — a third independent convergence on the same spine, reached by the least machinery.",
      ],
      buildCmd: "g++ -std=c++17 -O2 -o roadtrip main.cpp\n./roadtrip",
      outputs: [
        {
          title: "Westbound from Denver (hardcoded 59-place dataset)",
          text: "Best Westbound Route From Denver\nTotal Miles: 1100.69\nScore: 332.331\n\nStop 1: RockyMountainNP | Miles: 55.5359 | Cumulative: 55.5359\nStop 2: ArchesNP | Miles: 236.09 | Cumulative: 291.626\nStop 3: GrandCanyonNP | Miles: 231.952 | Cumulative: 523.578\nStop 4: BryceCanyonNP | Miles: 106.34 | Cumulative: 629.918\nStop 5: ZionNP | Miles: 50.3398 | Cumulative: 680.258\nStop 6: SequoiaNP | Miles: 311.155 | Cumulative: 991.413\nStop 7: YosemiteNP | Miles: 109.273 | Cumulative: 1100.69",
        },
      ],
      outputNote:
        "Same seven parks, mean rating 4.80 — with raw great-circle miles, so every leg understates real driving.",
    },
  },

  /* ---------------- HEAD-TO-HEAD ---------------- */
  comparison: {
    intro:
      "Four codebases, one problem, one dataset. The rows below are the study's own measurements; where outputs were re-run for this site, the numbers are from those runs.",
    main: {
      cols: ["", "My Project", "Claude Code", "Codex", "Copilot"],
      rows: [
        ["Search structure", "DP over (night, region)", "DFS + branch and bound", "DFS + backtracking", "DFS + backtracking"],
        ["Worst-case cost", "O(nights · n²)", "~6 billion paths", "O(n² + nᵏ)", "O(k · nᵏ)"],
        ["Exact optimum", "yes", "yes", "no", "yes"],
        ["Road factor", "1.25 (specified)", "1.18 (self-chosen)", "1.00 (great-circle)", "1.00 (great-circle)"],
        ["Min daily mileage", "50 mi, by measured sweep (originally 100)", "50 mi, unprompted", "none", "none"],
        ["Considered DP?", "chose it deliberately", "\u201cI never considered it\u201d", "no; justified skipping it", "no; justified skipping it due to data selection"],
      ],
    },
    results: {
      title: "Re-run on western-usa.txt (59 regions, 7 stops)",
      cols: ["", "My Project", "Claude Code", "Codex", "Copilot"],
      rows: [
        ["Mean stop rating", "4.80", "4.80", "4.69", "4.80"],
        ["Route", "RMNP → Arches → Grand Canyon → Bryce → Zion → Sequoia → Yosemite", "same seven parks", "Black Canyon → Arches → Canyonlands → Capitol Reef → Bryce → Zion → Yosemite", "same seven parks"],
        ["Reported total miles", "1,572.57", "1,299", "1,013.9", "1,100.69"],
        ["…at road factor", "× 1.25", "× 1.18", "× 1.00", "× 1.00"],
        ["Shortest day", "62.9 mi", "59 mi", "32 mi", "50.3 mi"],
        ["Reaches San Francisco", "yes — 196.71 mi final drive", "no — planner stops at last park", "yes — 157 mi final leg", "no — route ends at Yosemite"],
      ],
      note:
        "Reported miles are not directly comparable across road factors: ×1.00 totals quietly understate real driving. Three of four planners converge on the identical seven-park spine. And every route above, from every planner, is strictly decreasing in miles remaining to San Francisco — none of them was told that invariant, and none of them violates it.",
    },
    scaling: {
      title: "Why the algorithm choice matters as data grows",
      cols: ["places (n)", "search worst case P(n,7)", "DP at 7 · n²", "ratio"],
      rows: [
        ["14", "17,297,280", "1,372", "12,607×"],
        ["20", "390,700,800", "2,800", "139,536×"],
        ["40", "93,963,542,400", "11,200", "8,389,602×"],
        ["59 — my dataset", "1,719,393,207,840", "24,367", "70,562,367×"],
      ],
      note:
        "Worst-case route-search operations at 7 stops. Pruning is a practical speedup, not a guarantee — DFS with backtracking and branch and bound share the same P(n,7) ceiling; the DP runs at 7·n².",
    },
  },
};
