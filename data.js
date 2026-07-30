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
    author: "One project. Two generic prompts. Three AIs. Seven codebases.",
    tagline:
      "A study of how phrasing alone produces entirely different codebases for the same goal, and why knowing the algorithm before the prompt is what keeps a project on the road.",
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
    { id: "study",         label: "The Study" },
    { id: "original",      label: "My Project" },
    { id: "claude-code",   label: "Claude Code #1" },
    { id: "claude-code-2", label: "Claude Code #2" },
    { id: "codex",         label: "Codex #1" },
    { id: "codex-2",       label: "Codex #2" },
    { id: "copilot",       label: "CoPilot #1" },
    { id: "copilot-2",     label: "CoPilot #2" },
    { id: "compare",       label: "Head-to-Head" },
  ],

  /* ---------------- THE ESSAY ---------------- */
  blog: [
    {
      heading: null,
      html: `<p class="lede">Over the past several months, I have experimented with building software alongside AI, exploring how different prompts affect the structure, size, and comprehensibility of the generated code. This is an account of one project where that difference was stark enough to measure.</p>`,
    },
    {
      heading: "Abstract",
      html: `<p>One of my most recent fields of study is dynamic programming. With substantial classwork catered towards 1D Dynamic Programming, there was little practice/observation of 2D DP. To get that experience, and out of genuine curiosity, the scope of this project was geared towards the preliminary stages of finding an efficient and optimal solution for ideal routes and rankings based on a specific dataset. The final project includes two parts. The first, ROADTRIP, takes a selection of 59 hand-curated locations labeled with location name, latitude, longitude, and an opinionated star rating out of 5, with natural sightseeing locations rated the highest. The data contains cities, towns, and national parks across the western United States, as this project is tailored to finding an ideal road trip from Denver to San Francisco. Every stop has to leave less road remaining to the destination than the last one did, and that single constraint is what makes the rest work. A strict decrease means the graph is acyclic, so the DP never has to track which regions have already been visited. Each leg also has to fall inside a daily driving window, which was originally set to be at least 50 miles and at most 450 miles. This interval was decided because 450 miles equates to approximately 450 mi/60 mph = 7.5 hours of driving, which is a standard drive length. 50 miles serves as a limit to make every drive somewhat productive. The final project has been tested extensively for this default constraint. The final drive into the destination is bounded separately by a 700-mile limit, since a day of driving is a day you plan around, and arriving is not. The user is fully expected to input their accurate mileage limits and for them to actually allow the trip to be completed distance-wise, otherwise no route will be returned.</p><p>It uses a 2D DP that tracks dp[stops][region], tracking the best way to reach each region in a certain amount of stops while maximizing the mean star rating across exactly the number of nights requested. Once the table is completed, it employs a search across every region within reach of the destination to find the best route, all of which runs in O(nights · n²). The OPTLOOP (second part) employs 2D dynamic programming to solve the Travelling Salesperson Problem on a small set of data (15 locations), capped at 20 regions because the table grows as 2ⁿ · n and hits a memory wall past that rather than a time one. This involved the Held-Karp algorithm, which took extensive research to understand bit masking and the algorithm as a whole, and runs in O(2ⁿ · n²). The distance between those two complexities is the entire point. A one-way trip has an ordering to exploit and a loop does not. Both solvers are checked against a separate brute-force program that shares no search code, using exhaustive DFS for ROADTRIP and full permutation enumeration for OPTLOOP.</p><p>The scope of this entire project is intended to be the preliminary stages of a small subproblem with real capacities for translation to larger datasets. This paper is an investigation into how three AIs (Claude Code, Codex (ChatGPT), and Copilot) react to a prompt that describes this very problem in more generic terms, into holes in prompting, and into how prompting a small subproblem generically can lead to large holes in complexity as the coding project grows. This is an examination of these small holes and the importance of understanding algorithms before prompts in order to efficiently and correctly attack an intended project. Held-Karp is not included in the scope of this examination, as it arrived later in the project, after the decision to solve it via dynamic programming had already been made.</p>`,
    },
    {
      heading: "The first attempt",
      html: `<p>The idea came before the education. I wanted a program that took specific details about my own preferences (such as rigorous hikes, scenic views, modes of travel) and returned the places that best matched them, not the most popular option but the most suitable one. Nature and sightseeing seemed like the most specific version of the problem, and one I genuinely enjoyed. If I want to go on a walk or spend a day at a lake, how do you turn “best place to go” into something computable? Topography, review scores, hike difficulty ratings. Eventually I wanted the same engine to work on restaurants, campsites, movies, road trips, anything with enough attributes to rank.</p><p>At the time I knew very little about algorithms. I asked Claude Code to build an algorithm for a hiking trail ranking system.</p><p>What came back was over a thousand lines of code, most of it machinery for pulling map data from external APIs. The output looked impressive, and my first instinct was to assume it worked, that the code was doing what I had pictured. Logically I knew I couldn’t have described the scope of the problem in the handful of rushed, unspecific prompts I had given it. The program appeared to be returning desirable hikes, though I couldn't read it well enough to confirm that. That gap was the frustrating part. I couldn't tell whether I had what I asked for, and after a solid amount of observation, I realized the code output was not giving me any real values for which desirable hikes to try. As it turned out, the code evaluated almost none of what I cared about. It was returning hiking routes filtered by mileage.</p><p>A few computer science courses later, I understood the failure differently. The problem wasn't an inability to read code I hadn't written. It was that I had given myself no patterns to recognize, because I had never specified any. The first version ran off in directions I couldn't follow because I never told it where to go.</p>`,
    },
    {
      heading: "Narrowing the problem",
      html: `<p>Some months later I dropped the hiking version entirely. The issue was that the data needed required extensive map APIs and caused overcomplication of a problem that needed only data analysis, not data retrieval. So I narrowed the scope to something I could reason about end to end, a cross-country road trip planner covering several nights, with a hard cap on how far I would drive in a day. It would read everything from text files rather than the internet, which removed the API machinery that had swallowed the first attempt. The motivation was practical. I was about to drive across the country, and I wanted to see whether I could build something that would generate a genuinely good itinerary of places to camp along the way. With a better grasp of algorithms, I also knew the optimization itself didn't require complicated code, and I wanted to find out whether Dynamic Programming, which I had just finished studying, could solve it in a form short enough to read.</p>`,
    },
    {
      heading: "The specification",
      html: `<p>I wrote the request as a specification. Instead of describing an outcome, I named the algorithms I wanted used. The first was Dynamic Programming over a Directed Acyclic Graph (DAG) because this road trip only progresses in one direction, which makes the graph acyclic and the DP cheap as the linear sort guarantees each subproblem is evaluated exactly once. Then the second part was a Dynamic Programming solution to the traveling salesperson problem for optimal loop routing on shorter trips, rather than a heuristic one.</p><p>I worked extensively with Claude to ensure the project accepted a broad set of limitations and data, avoided unexplained constants where possible, and maintained simplicity, as AI tends to try and solve everything all at once. I could immediately identify the sections that weren't doing what I originally planned, the thing the hiking project had made much more difficult. After sharpening the code and studying the Held-Karp algorithm to handle a new TSP side of the problem, I had a working skeleton of the idea I'd had months earlier, a program that traverses arbitrary data points and returns the best results for a specified set of preferences. There is a great deal left to build on top of it, which I plan on adding at some point, but the backbone holds.</p>`,
    },
    {
      heading: "The experiment (Prompt #1)",
      html: `<p>The goal was to test this code generation with specific and general prompts. The carefully coded and prompted final project stands as the representative of specific, intentional prompting, compared against the generic prompts listed below run on several AIs (Claude Code, Codex, Copilot). The question became how this specific project's time complexity, space complexity, and code auditability compared to a generic prompting of AIs.</p><p>Every run attached the same file the project uses, western-usa.txt, 56 regions between Denver and San Francisco at the time of the original runs, since grown to 59 to make routing possible. Every rerun and every table in this paper uses the current 59-region file, and the original one-run outputs were re-verified against it. After prompting, this file was given to the AIs to adjust their formatting to this input file and return their results.</p><p>Prompt #1:</p><blockquote class="prompt-block">Create a C++ project that finds me 4-7 of the best spots to visit on a road trip that can be 4-7 days (4-7 is the range the constant can be in). depending on the destination, you can only move east or west, never go the opposite way, but you can drive up or down. You cannot exceed 450 miles in one day, and you must return the most ideal trip with the coolest natural places to visit (national parks/ natural novelties are rated higher than cities). the best route possible should be returned with location, stop number, miles to that location from the last, and headed with total miles driven</blockquote><p>For the sake of this project, this prompt was generated using my own personal practicality usage, as I felt this way the question sat more generically in relation to how a person might prompt an AI when vibe coding. The parameters of 4-7 days were set because it’s pretty typical of how long a road trip could be, and the scope of the prompt suggested it would be a program that was applicable to all road trips.</p><p>Two of the prompt's numbers deserve flagging, because they became this study's controlled variables rather than its observations. The 4-7 day range and the 450-mile ceiling were both written into the generic prompt, so every model received them as requirements. All three then compiled them in as fixed constants instead of exposing them as inputs. Not one of them made either value adjustable at runtime. My own project is the only one of the four that takes both as command-line flags, which is the reason the sweeps later in this paper are possible on my code and nowhere else. It also means every comparison here has to happen at the one configuration all four programs can express. That is a constraint built into the experiment, not a shortcut in running it.</p><p>(NOTE: In the original project creation, the Held-Karp algorithm was not planned so it was not prompted here, as that specific subproblem gives away that this project should be dynamically programmed. Held-Karp was an evolved state of this project.)</p>`,
    },
    {
      heading: "What came back",
      html: `<p>The answers diverged. Codex and CoPilot proposed depth-first search with backtracking; Claude proposed DFS with branch and bound. All three produced working code, each optimal against its own objective, but all of them are exponential in the worst case. The DAG formulation I'd specified runs in polynomial time, which made it far cheaper than any of the three.</p><div class="table-scroll"><table>
<thead><tr><th>places (n)</th><th>search worst case P(n,7)</th><th>DP at 7 · n²</th><th>ratio</th></tr></thead>
<tbody>
<tr><td>14</td><td>17,297,280</td><td>1,372</td><td>12,607×</td></tr>
<tr><td>20</td><td>390,700,800</td><td>2,800</td><td>139,536×</td></tr>
<tr><td>40</td><td>93,963,542,400</td><td>11,200</td><td>8,389,602×</td></tr>
<tr><td>59 (my dataset size)</td><td>1,719,393,207,840</td><td>24,367</td><td>70,562,367×</td></tr>
</tbody></table></div><p class="fig-caption">Worst-case route-search operations at 7 stops. DFS with backtracking and DFS with branch and bound share the same P(n,7) worst case, since pruning is a practical speedup rather than a guarantee; the DP runs at 7 times n squared.</p><p>My original prompt never specified an input format, and both models filled that gap with a data layer I didn't ask for. An unspecified input format does not produce less code. It produces more.</p><p>Three smaller observations from the outputs. First, Codex used raw great-circle distance with no road factor at all, so its reported 1,118-mile trial total quietly understates real driving on every leg. Second, both models invented a mileage penalty in their objective without being asked and picked very different weights for it. Codex scored route_value = score − 0.015 × miles; Claude Code first calibrated 0.0015 and then recalibrated down to 0.000001, four orders of magnitude below Codex, each constant hand-picked and unexplained. A convergent structure with arbitrary numbers. These arbitrary calculations reduced the soundness of the actual calculations, although the routes returned were high-scoring routes. Third, the minimum-mileage problem showed up in both outputs from opposite directions. Claude Code noticed it and patched it unprompted with a 50-mile floor. Codex never noticed, and its own best route contains a 32-mile “day” from Arches to Canyonlands. I had originally hit the same flaw and set a 100-mile floor by design; later, after sweeping the floor against attainable score, I moved it to 50, the same number Claude Code had picked unprompted.</p><p>That was the satisfying part. I had shaped an algorithm that was correct, efficient, and assembled from patterns I already knew. This success led to a question that prompted my work on this project. Why does phrasing alone produce entirely different codebases for the same underlying goal?</p>`,
    },
    {
      heading: "Why none of the answers were wrong",
      html: `<p>What is to be learned is that it doesn't come down to the AI's strengths at this point, it comes down specifically to the holes that a prompt may have. The challenge, however, was that none of the AI outputs to the question were incorrect. They all provided the optimal answer to the question in their own datasets (scoring for the final project’s data was more optimal than Codex's, but that is to be expected based on the specified requirements I had made). The generic prompt reads as a textbook TSP problem via unordered search, and branch and bound and DFS with backtracking are textbook solutions to that problem. So was it that my prompt was too generic?</p><p>Fascinatingly enough, that is actually not the full story. My error in running this trial of comparison is that I did not consider the fact that even my generic prompt was just a slightly dumbed down version of an actual explanation of my DAG DP problem. There is literally only a single phrase in the entire prompt that the AI interpreted differently that kept it from solving via DP: “you can only move east or west, never go the opposite way, but you can drive up or down.” If that sentence is changed to “Every stop has to bring me closer to the destination than the last one did,” you get the AI returning a DAG DP solution a lot of the time.</p><p>For the original prompt, it contained the same rule, you can't go backwards. The generic prompt states it as a prohibition, a move that isn't allowed, and a prohibition reads as a filter you apply while searching. That is exactly what branch and bound and DFS with backtracking are. Changing that one sentence shifted the narrative from prohibition to an ordering, changing the problem entirely.</p><p>What this means is that in this small sample, we see a situation where I, the user, was mere words away from solving with a different, less efficient algorithm. This highlights the risk of prompting, and how a knowledge of what code you plan to use can keep your final product from deviating into different algorithms and methods. In this case, our code only ended up less efficient, but in larger products this could result in complete differentiation in functionality. This sheds light on the importance of knowing where your program is supposed to be going, so you can quickly readjust your code when AI misinterprets a few words in your prompt. Asking your agent what algorithms it plans on employing before the code gets written can help catch deviations earlier.</p>`,
    },
    {
      heading: "What it costs when you can't read the answer",
      html: `<p>So how can I make one good prompt without having to spend hours arguing with AI over what I really want the project to look like in my mind? Well, it seems that I must know exactly how to code the problem in order to direct AI properly. The end product of my refined project was not difficult to explain on paper. AI served as a tool to collapse execution time, but not thinking time. Every word had to come as a calculation, and my education gave me the foundation to invest that time in critical thinking. When I asked Claude Code (generic prompting) why it did not use DP optimization, it returned that it did not even consider it. It identified the problem it wanted to solve, and once it realized it worked, it stopped searching for better solutions. Its own words:</p><blockquote class="prompt-block">“I didn't weigh DP and reject it. I never considered it. The README documents the branch-and-bound thoroughly across seven design sections and doesn't mention dynamic programming once. That absence is the tell. It wasn't a trade-off, it was a reflex.”</blockquote><figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Claude Code &middot; follow-up run</div>
    <img src="images/claude-code-why-not-dp.png" width="1250" height="832" loading="lazy" alt="Claude Code transcript. Asked to state just the algorithm used, it answers DFS with branch and bound. Asked why it did not use dynamic programming via DAG, it answers that there is no principled reason, that it never considered it, that it had already done the DAG reduction and ran an exhaustive search over the DAG anyway, and that the state would have been (node, stops used).">
  </div>
  <figcaption>Claude Code's response to why it did not utilize dynamic programming.</figcaption>
</figure><p>Pressed further, it found the evidence had been sitting in its own design note the whole time. Its bounding function, a suffix table built back-to-front by tabulation, is itself a Dynamic Program. It wrote a DP to prune a search over a problem that was already a DP. And it named the one-minute audit it had skipped, to go through each constraint and ask whether it is path-cumulative or edge-local. Every constraint in my problem is edge-local, so the state collapses to (current place, stops used), and the whole thing is a DP. By Claude’s own account, it did not arrive there independently.</p><p>Pressed on the same question in a follow-up run, it went further. “There's no principled reason. I didn't evaluate it and reject it — I never considered it.” It had already done the DAG reduction itself and written it down as the key move, then ran an exhaustive search over the DAG anyway, “the one thing the reduction exists to make unnecessary.” It also diagnosed why the reflex fired. The exactly-K-stops requirement made the problem read as not quite the textbook DAG longest-path, and the standard fix, adding stop count as a second state dimension, dp[k][v] over a layered DAG, is precisely the state my project uses.</p><figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Codex &middot; ChatGPT</div>
    <img src="images/codex-why-not-dp.png" width="926" height="1269" loading="lazy" alt="Codex transcript. Asked why it did not use dynamic programming, it answers that the problem was small enough that DFS with backtracking and pruning was simpler and still fast, and lists the state DP would need: current location, number of stops chosen, which places have been visited, direction constraints, and score versus mileage tradeoff.">
  </div>
  <figcaption>Codex's response to why it did not utilize dynamic programming.</figcaption>
</figure><p>Codex, asked the same question, produced a justification rather than a concession. DP, it said, would be more complex because the state would need to track the current location, the number of stops chosen, which places have been visited, the direction constraint, and the score-versus-mileage tradeoff. It stated that the smaller data did not need more than DFS directly. Three of those five do not belong in the state at all. The one-direction rule is exactly what removes the visited set, the direction constraint folds into the index ordering, and an additive mileage penalty folds into the edge weight. Its explanation for skipping DP rested on state the problem never needed.</p><figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Microsoft Copilot</div>
    <img src="images/copilot-why-not-dp.png" width="756" height="1114" loading="lazy" alt="Copilot transcript. Asked why it did not use dynamic programming, it lists five reasons under the heading Why DFS instead of DP: direction lock, variable trip length, scoring system, constraint pruning, and search space size.">
  </div>
  <figcaption>CoPilot's response to why it did not utilize dynamic programming.</figcaption>
</figure><p>CoPilot returned the most concise explanation of why it did not use Dynamic Programming at all, highlighting the scope of the input as the leading factor in how it decided which program to run, as well as directional constraints.</p><p>If unaware of the algorithms that could solve the problem, the user would never be able to orient the AI toward the answer they actually wanted. And if it returned code the user couldn't read, the user would struggle to explain that it isn't what they’re after and how to fix it. The problem is right there in how stops weren’t being ordered.</p><p>This happened in the first working version of the final project, the program decided whether one stop could follow another by comparing how far each sat from Denver. Farther from the start counted as progress. That is not the same thing as getting closer to where you're going, and the difference showed up as routes detouring hundreds of miles north before turning back west. One went Arches, then Jackson Hole, then Bryce Canyon, each stop farther from Denver than the last, while the middle one left me 276 miles farther from the destination of that version, Joshua Tree, than the stop before it. Nothing in the code was broken. It compared the wrong two numbers, and it compared them because that is what I had asked for. My own constraint said never move back east, and every one of those stops was west of the last. Finding it meant understanding that the ordering was the thing making the graph acyclic, and that changing which point it measured from would keep that property while fixing the behavior. It was not AI's shortcoming here, it was the gaps in my specifications. And this bug alone helped me gain a much better understanding of the problem.</p><div class="table-scroll"><table>
<thead><tr><th></th><th>Arches</th><th>Jackson Hole</th><th>Bryce Canyon</th><th>verdict</th></tr></thead>
<tbody>
<tr><td>miles from Denver</td><td>320</td><td>493</td><td>519</td><td>all increasing: legal</td></tr>
<tr><td>longitude</td><td>−109.59</td><td>−110.76</td><td>−112.19</td><td>all westward: legal</td></tr>
<tr><td>miles remaining</td><td>613</td><td>889</td><td>415</td><td>Jackson Hole is backward</td></tr>
</tbody></table></div><p>Neither the original constraint (east or west only) nor the implementation's axis (distance from origin) catches the detour. Only remaining distance to the destination does, and that became the fix. The ordering axis flipped from distance-from-origin, increasing, to distance-to-destination, strictly decreasing. Same acyclicity, correct meaning of progress.</p>`,
    },
    {
      heading: "Results",
      html: `<p>DISCLAIMER: During the running of the actual one-run outputs of the generic prompting, I did nothing to add to the prompt to adjust it. The difficulty of making adjustments is not factored in here, and instead my polished project is simply compared to the direct results from the other projects adjusted to using my western-usa.txt file. The trip14.txt file was not used by the other AIs, neither was OPTLOOP. I mention these below in my analysis of my polished project to further explain edits I made to my code and input files to correctly process my intended outputs.</p><p>What follows is a case study rather than a controlled experiment, and the difference matters for what can honestly be drawn from it. Each model was prompted once and its first output taken as final, so there is one sample per model and no way to separate a model's tendency from a single run's variance. All AI output was measured against a single dataset. The claims here are therefore about what these four programs do, not about what these models do in general. Where results converge across implementations that share no code, I have said so, because agreement between independent searches is the strongest evidence available at this sample size. The soundness of the numbers rests on verification rather than volume. Every figure reported for my own project is confirmed by an exhaustive checker that shares no solver code, so what this paper trades in statistical breadth it holds in proof.</p><h3>Every trip length, all four implementations</h3><p>To widen the comparison past the single 7-stop run, every implementation was rerun at each trip length it accepts. Three things come out of it. My solver and Claude Code return identical mean ratings at all four lengths (4.85, 4.84, 4.82, 4.80). Two independent searches agreeing at every length is the clearest evidence I have that both are finding the true optimum rather than a shared mistake. Codex separates from them as the trip lengthens, 4.83 down to 4.69, because its mileage penalty trades stop quality for shorter days and that cost compounds with every stop added. What looked like a single weak result at 7 stops is actually a slope. CoPilot cannot be tested at any other length at all, since it takes no arguments and prints only the winner of its internal 4-7 search.</p><p>Outside that range the four programs stop being comparable, and how they stop is itself the result. Codex rejects any request below four stops with an error, while Claude Code and CoPilot have no interface to make one because their trip length is a compiled-in constant. My project simply answers, 4.83 at three stops and 4.80 at two. At one stop it returns no route, and that is the correct answer rather than a shortfall. Denver to San Francisco is 1,184 road miles across two driving legs, so a 450-mile ceiling caps total progress at 900, and no one-stop trip exists for any planner, however it searches. The difference is that mine is the only one of the four that can be asked the question and return the true answer, because its limits are arguments rather than constants.</p><p>That is the intended contract. My project is built for intentional user input and carries no automatic “most optimal” mileage limit of its own, which is the next phase of this problem. Where the constraints and the data given admit no legal stop, it says so instead of relaxing a limit the user chose.</p><div class="table-scroll"><table><thead><tr><th>stops</th><th>My project</th><th>Claude Code</th><th>Codex</th><th>CoPilot</th></tr></thead><tbody><tr><td>4</td><td>4.85 · 1,379 mi</td><td>4.85 · 1,116 mi</td><td>4.83 · 984 mi</td><td>n/a</td></tr><tr><td>5</td><td>4.84 · 1,451 mi</td><td>4.84 · 1,184 mi</td><td>4.78 · 990 mi</td><td>n/a</td></tr><tr><td>6</td><td>4.82 · 1,528 mi</td><td>4.82 · 1,226 mi</td><td>4.73 · 1,001 mi</td><td>n/a</td></tr><tr><td>7</td><td>4.80 · 1,573 mi</td><td>4.80 · 1,299 mi</td><td>4.69 · 1,014 mi</td><td>4.80 · 1,101 mi</td></tr></tbody></table></div><p class="fig-caption">Mean site rating (0-5) and total miles across the 4-7 range the prompt defined, western-usa.txt (59 regions), 450-mile daily ceiling throughout. “n/a” marks a length CoPilot has no interface to request. Mileage differs between my project and Claude Code at equal ratings because of road factor, 1.25 against 1.18, not because the routes differ.</p><div class="table-scroll"><table><thead><tr><th></th><th>route selected at 7 stops</th></tr></thead><tbody><tr><td>My project</td><td>Rocky Mtn › Arches › Grand Canyon › Bryce › Zion › Sequoia › Yosemite</td></tr><tr><td>Claude Code</td><td>Rocky Mtn › Arches › Grand Canyon › Bryce › Zion › Sequoia › Yosemite</td></tr><tr><td>Codex</td><td>Black Canyon › Arches › Canyonlands › Capitol Reef › Bryce › Zion › Yosemite</td></tr><tr><td>CoPilot</td><td>Rocky Mtn › Arches › Grand Canyon › Bryce › Zion › Sequoia › Yosemite</td></tr></tbody></table></div><p class="fig-caption">The same seven-park route from three independent searches, a dynamic program, a branch and bound, and a depth-limited DFS. Codex is the lone divergence. Its low-mileage incentive pulls it onto Canyonlands and Capitol Reef, two shorter hops with lower ratings, which is what costs it the 0.11. Every route here is strictly decreasing in miles remaining to San Francisco, an invariant none of them was told about.</p><p>These tables are the paper's argument in miniature. The generic programs can only answer at the lengths their prompt happened to name, because a prompt's numbers harden into constants. The specified project kept the same numbers as inputs, so one codebase sweeps the whole range and explains its single refusal with arithmetic. And where the searches agree, with identical ratings at every shared length and the same seven-park spine from three algorithms sharing no code, all confirmed by the brute-force checker, that convergence is what a single-run study has instead of sample size. It is enough to carry the claim this paper actually makes. The difference between these four codebases was never the models. It was the specification.</p><h3>The final project</h3><p>Exact output from the final project ROADTRIP mode on western-usa.txt (59 regions, defaults of 7 nights, 50-450 mi daily, 700 mi final approach). Score 4.80 (mean site rating), 1,572.57 total miles including the 196.71-mile final drive, the optimum under the 450 ceiling. Every stop is a National Park.</p><figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">My project &middot; bestRoute on western-usa.txt</div>
    <img src="images/original-bestroute-output.png" width="1476" height="868" loading="lazy" alt="Roadtrip mode output on the western dataset. Path Denver, Rocky Mountain NP, Arches NP, Grand Canyon NP, Bryce Canyon NP, Zion NP, Sequoia NP, Yosemite NP, San Francisco. Total 1572.57 miles, mean site score 4.80, with a mi-left-to-SF column reading 1211.66, 1142.24, 873.15, 724.80, 699.21, 644.34, 287.71, 196.71, 0.">
  </div>
  <figcaption>The mi-left column is strictly decreasing down the list, 1142 → 873 → 724 → 699 → 644 → 287 → 196: the monotone progress invariant visible right in the output.</figcaption>
</figure><p>The mi-left column is strictly decreasing down the list, 1142 → 873 → 724 → 699 → 644 → 287 → 196, the monotone progress invariant visible right in the output.</p><p>The floor stays at 50, a practical limit for how much should be driven daily. The data was at times too sparse, resulting in frequent route finding failures. The diagnosis was to just extend the final drive’s length allowance into San Francisco and add more values to the data set. This is practical because there is nothing to do on the last day of driving except reach the destination. The flags are now --lo 50 for the shortest daily drive, --hi 450 for the longest, and --final 700 for the final approach, which also caps the loop mode's leg home. I later added Yosemite to trip14 itself, which puts a region 197 miles from the destination and removes the starvation from the data side as well. This is a quintessential point in the project, because this data adding goes to show that the selective data used is equally as important as the actual code. Without Yosemite and data points that allow for shorter drives in OPTLOOP, the program fails and the code to fix it becomes altogether much more complicated. It also highlights the next phase of the project: to find the optimal route limitations given the best route-to-mileage score. This was a learning curve in AI usage, learning how to keep AI from solving every single problem when the problem should be much smaller. That fix being to use carefully curated data. Yosemite (4.9) and Sequoia (4.7), the two closest regions to San Francisco in the file, fill the final legs and let the trip finish 196 miles out instead of 644. Against the original 56-region file, the score went 4.71 → 4.80 and total mileage 2,442 → 1,573. Making the final product less exhaustive yet just as optimal. So this project works better the more data allotted.</p><div class="table-scroll"><table>
<thead><tr><th></th><th>single --hi 450</th><th>split 450 / 700</th></tr></thead>
<tbody>
<tr><td>western-usa ROADTRIP</td><td>4.80</td><td>4.80</td></tr>
<tr><td>trip14 OPTLOOP</td><td>2732.12</td><td>2732.12</td></tr>
</tbody></table></div><h3>Claude Code</h3><p>To make the numbers comparable, all models were rerun against western-usa.txt and the 0-5 rating scale. Claude Code, adapted to western-usa.txt with the rating used directly, returned a 7-stop route at a 4.80 mean rating and 1299 miles, longest day 367, shortest 59. One caveat, Claude Code’s road factor is 1.18 against my 1.25, so even with the window now matched at a 450 cap and 50 floor, the edge sets are not quite identical.</p><figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Claude Code &middot; rerun on western-usa.txt</div>
    <img src="images/claude-code-route-rerun.png" width="1486" height="766" loading="lazy" alt="Claude Code output: 7 days, 7 stops, 1299 miles, mean rating 4.80, longest day 367 miles against a 450 cap. Stops are Rocky Mountain NP, Arches NP, Grand Canyon NP, Bryce Canyon NP, Zion NP, Sequoia NP, and Yosemite NP.">
  </div>
  <figcaption>Claude Code rerun against western-usa.txt on the 0–5 scale: the same seven parks, 1,299 miles, 4.80 mean.</figcaption>
</figure><h3>Codex</h3><p>Codex's scoring logic on the same dataset produced a different character of route. As seen below, it selects a series of much shorter routes, travelling 32 miles, 70 miles, 77 miles, and then 50 miles to a few lower-scoring areas, leading to an average location scoring of 4.69. This happened because of there being no lower mileage cap and Codex generating a low-mileage incentive. These are inventions in code that are not practical for my project's scope or its readability.</p><figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Codex &middot; rerun on western-usa.txt</div>
    <img src="images/codex-route-rerun.png" width="1480" height="848" loading="lazy" alt="Codex output: Denver to San Francisco, 1013.9 miles, place score 32.8 of 35, route value 17.592. Stops are Black Canyon NP, Arches NP, Canyonlands NP at 32 miles from the last stop, Capitol Reef NP at 70, Bryce Canyon NP at 77, Zion NP at 50, and Yosemite NP at 359.">
  </div>
  <figcaption>Codex's route on the same dataset — the short 32, 70, 77 and 50-mile legs are the days its objective was willing to spend on lower-rated stops.</figcaption>
</figure><h3>Microsoft CoPilot</h3><p>Microsoft CoPilot used depth-limited exhaustive DFS with backtracking and feasibility pruning, resulting in the optimal solution in a single main.cpp. Although it holds no low mileage restriction, it does not incentivize low mileage, allowing for an optimal route path.</p><figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Microsoft Copilot &middot; rerun on western-usa.txt</div>
    <img src="images/copilot-route-rerun.png" width="1482" height="764" loading="lazy" alt="Copilot output: best westbound route from Denver, 1100.69 miles, score 332.331. Stops are Rocky Mountain NP, Arches NP, Grand Canyon NP, Bryce Canyon NP, Zion NP, Sequoia NP, and Yosemite NP.">
  </div>
  <figcaption>CoPilot's best westbound route from Denver: the same seven parks again, 1,100.69 miles, out of 180 lines of a single main.cpp.</figcaption>
</figure><p>Proof of functionality sits in the brute force route findings of the 59-region file. The final project’s own checker, and these AIs' DFS runs, confirm the optimal route of the same 4.80 mean with the same five-park spine of Rocky Mountain, Arches, Grand Canyon, Bryce, and Zion. One more convergence the unified tables make visible is that every AI's route in this section, from every planner, is strictly decreasing in miles remaining to San Francisco, and none of them was told that invariant, and none of them violates it.</p>`,
    },
    {
      heading: "Proving it right",
      html: `<p>The generic prompt's algorithms did not go to waste. Exhaustive search is a terrible way to solve this problem and a perfect way to check it, so the naive approach became the verifier, a second program that finds the answer by brute force and gets diffed against the solver on hundreds of random inputs. The two programs share the parsing and the distance math out of necessity, but nothing about how they search.</p><p>The diff testing caught exactly one real disagreement, and it was worth the whole harness. On one dataset the solver and the checker returned identically scored routes through different stops, the solver went through Grand Canyon, the checker through Zion, both rated the same. Neither was wrong. The solver breaks ties between equal states toward the larger node, which cascades into preferring the route that is lexicographically larger, and my checker had no opinion on ties at all. The mismatch existed because a rule was implicit in one program and absent in the other. The fix was stating the tie-break policy explicitly in both, and the lesson is that a verifier does not just catch wrong answers, it forces every unstated decision out into the open.</p><p>Re-run against the final architecture at the split defaults, 1400 of 1400 random ROADTRIP instances match the exhaustive DFS on full output, and 200 of 200 OPTLOOP instances match the permutation check.</p>`,
    },
    {
      heading: "The Second Prompting",
      html: `<p>After analyzing the processes that were returned from the first generic prompt, such as particular limitations and lack of clarity in the ordering of the locations, there was room to add one more test related to what was learned about prompting. A second generic prompt, this time phrasing the constraint as an ordering rather than a prohibition, and asking for a much broader sense of the data the program may be required to handle.</p><p>Prompt #2:</p><blockquote class="prompt-block">Create a C++ project that finds me the best spots to visit on a road trip of any particular length. depending on the destination, you can only move toward the destination, never go the opposite way, but you can go down a route not necessarily a direct shot to the destination. You cannot exceed 450 miles in one day, and you must return the most ideal trip with the coolest natural places to visit (national parks/ natural novelties are rated higher than cities). The user must be able to enter different start and destination locations. The best route possible should be returned with location, stop number, miles to that location from the last, and headed with total miles driven.</blockquote><p>The AIs had made it clear these distinctions would have allowed them to solve it with Dynamic Programming on the first go, so their memories were erased (with no definitive proof the memory was fully erased), and the short, slightly clearer prompt was given. DP was the expected algorithm. Whether the functions worked properly in one run was the focal point of interest.</p>`,
    },
    {
      heading: "AI's New Response",
      html: `<p>Claude Code showed the most substantial differentiation, and the most breadth. While at first attempting to use an unbounded DP, on its own it changed to a bounded DP with DAG. It returned a four-header project rather than a single file. Geometry, a Location type, a CSV-backed location database with free-text name resolution, and the optimizer itself. The optimizer is the same machinery as my final project, haversine distances, DP over a progress-ordered DAG, and the route retraced through parent pointers. Two genuine improvements over its first attempt stand out. The destination is now a mandatory final node rather than a compass heading, which also puts the closing leg under the same 450-mile cap as every other day. It also independently derived the feasibility floor this paper arrived at for my own solver. Asked for an impossible trip, it refuses with the arithmetic, “needs at least 3 day(s) for the direct distance alone,” and its suggested default trip length is that floor plus slack. The breadth is where the flaws live. Rather than read my file, it manifested a 163-location gazetteer of the entire country in one pass. It incorporated a “NATURAL_BONUS” which inflates every national park, but excludes cool towns that may receive equal ratings. Nothing in it holds an opinion about mileage. The only mileage term is a 1e-5 tie-break, so on my data it detours through Yellowstone, 575 extra miles to gain 0.1 of rating, which is exactly the error the final project encountered. The guard against absurdity is the user's day count, and nothing else.</p><p>Codex used DP. Its planner sorts every candidate by projected progress along the start-to-destination axis and relaxes each node against all predecessors, a longest-path pass over a DAG. But it wrapped that solid structure in a rating system full of numbers that appear slightly imaginative. Taken from its source, a park or natural wonder is valued at</p><blockquote class="prompt-block">rating × 80<br>+ category bonus (28 natural wonder, 24 park)<br>− 0.055 × miles off the direct route</blockquote><p>with cities cut to rating × 8 so they serve only as overnight connectors, and every leg charged a further 0.10 per detour mile plus a flat 7 per stop. Codex provided no indication of where any of those constants came from. The final ratings look great for the selected data, and that is precisely the problem. AI can mass-evaluate its own outputs and return an equation that fits its dataset very well. Even if the constants were all correct, they add a hard-to-follow process to the codebase, making it very difficult to identify what to fix when the program starts spewing incorrect values. Codex’s reach for several invented equations is a red flag.</p><p>CoPilot went the opposite direction. One file again, interactive prompts for exact location names, and, notably, it transcribed all 59 sites of my dataset into the source as an initializer list, faithful to the digit, with no file input at all. The faithful copy still cannot run this study's trip, because San Francisco was never a named site in western-usa.txt, only the destination coordinate in the header, and the program offers no way to enter a coordinate. On the nearest possible trip, Denver to Yosemite, it chose its own trip length (there is no stop-count input) and returned an 18-stop route in which eleven stops are towns. Its mileage penalty of 0.2 per hundred miles makes any town within reach a net scoring gain, so the route stuffs itself. The search is DFS with backtracking plus a pruning cutoff that is not admissible. Any branch running 1.0 behind the current best is discarded, though a branch can recover far more than 1.0 downstream. Because its strict forward rule makes the graph a DAG, its exact objective can be solved exactly with a DP, and the comparison cuts both ways. The true optimum of its own scoring is a 54-stop, 8,327-mile run scoring 187.8, and its DFS returned 61.4. Its objective rewards visiting nearly everything, and its search is too broken to find what the objective rewards. The output only looks like a road trip because the two defects point in opposite directions.</p><p>Each program was then checked against the western-usa data as far as its interface allows. Codex reads the file directly, Claude Code reads it converted to its CSV schema with San Francisco appended at rating zero as the mandatory destination row, and CoPilot carries its transcription of the sites internally and cannot be pointed at San Francisco at all.</p>`,
    },
    {
      heading: "Results (Prompt #2)",
      html: `<p>Below are the best route scores returned for each number of stops. Note that Codex and CoPilot did not fully understand the prompt and decided on their own ideal route sizes, where Codex returned 15 stops and CoPilot returned 18. This demonstrates the structure that is lost via prompting once the scope starts getting too broad, and shows a tradeoff between more accurate algorithm usage and drifted projects.</p><div class="table-scroll"><table><thead><tr><th>stops</th><th>My project</th><th>Claude Code #2</th><th>Codex #2</th><th>CoPilot #2</th></tr></thead><tbody><tr><td>1</td><td>no route</td><td>no route, floor printed</td><td>no length input</td><td>no length input</td></tr><tr><td>2</td><td>4.80 · 1,229 mi</td><td>4.85 · 975 mi</td><td>no length input</td><td>no length input</td></tr><tr><td>3</td><td>4.83 · 1,229 mi</td><td>4.87 · 982 mi</td><td>no length input</td><td>no length input</td></tr><tr><td>4</td><td>4.85 · 1,379 mi</td><td>4.85 · 1,103 mi</td><td>no length input</td><td>no length input</td></tr><tr><td>5</td><td>4.84 · 1,451 mi</td><td>4.84 · 1,160 mi</td><td>no length input</td><td>no length input</td></tr><tr><td>6</td><td>4.82 · 1,528 mi</td><td>4.83 · 1,734 mi</td><td>no length input</td><td>no length input</td></tr><tr><td>7</td><td>4.80 · 1,573 mi</td><td>4.81 · 1,734 mi</td><td>no length input</td><td>no length input</td></tr><tr><td>15</td><td>4.63 · 2,587 mi</td><td>4.66 · 2,418 mi</td><td>4.59 · 1,876 mi (its pick)</td><td>n/a</td></tr><tr><td>18</td><td>4.56 · 3,625 mi</td><td>4.59 · 2,424 mi (stops at 17)</td><td>n/a</td><td>3.56 · 1,305 mi (its pick, to Yosemite)</td></tr></tbody></table></div><p class="fig-caption">Mean site rating (0-5) and total miles by scenic-stop count under Prompt #2, all on the western-usa data. Mileage bases differ. My project counts road miles (great-circle × 1.25) while the prompt-2 programs count raw great-circle miles, so identical “450-mile” caps admit different real legs. Claude Code #2's day count includes the arrival day, and rows here are aligned by scenic stops. CoPilot #2 cannot be asked for San Francisco, so its row drives Denver to Yosemite, at its own length choice.</p><p>The cells that beat my column still mark looser problems, but the looseness moved somewhere subtler than the first round. At two and three stops, Claude's 4.85 and 4.87 are routes my solver is forbidden to drive. Both programs enforce “450,” but mine caps road miles (great-circle × 1.25) and Claude's caps raw great-circle miles, so its window admits legs up to 562 road-equivalent. Denver to Bryce is 415 great-circle but 519 road, legal there, illegal here. If my project’s ceiling is raised to 563 miles, it returns identical scoring. At six and seven stops the mileage column gives the game away. Its total jumps from 1,160 to 1,734 miles, the Yellowstone detour, to buy 0.01 of mean rating, because nothing in its objective charges for distance. A very similar problem I had to work through in my own project.</p><p>The self-chosen rows are where the second prompt's interpretations split. Codex still accepts no length and still answers fifteen. CoPilot also accepts no length and answers eighteen, a 3.56-mean route through eleven towns, on a trip that needed a substitute destination to run at all. Codex’s magic-number scoring produces different values and different stops. CoPilot's mileage preferences favor more stops and less travelling.</p><p>There is room for skepticism in my own project, as it was engineered with Claude, and Claude Code is consistently tracking similar values. It is possible that Claude did not actually erase its memory when requested. Either way, it's evident that results fly around when the prompt is made broader, come back more accurate when the scope is smaller, and in all cases still require a lot of coding work and analysis just to steer the function toward your exact intentions.</p>`,
    },
    {
      heading: "About my project: Held-Karp",
      html: `<p>My project contains its own piece of independent study, the Held-Karp algorithm. This was just a secondary investigation that came during the coding of my project to verify a solution to TSP via DP. OPTLOOP, the mode name, has no direction of progress, unlike ROADTRIP. A trip that starts and ends at home has no ordering to exploit, so the places already visited have to live in the algorithm's state, and the right tool is Held-Karp, dynamic programming over subsets with a bitmask. One project, two subproblems, and the structure that makes the corridor cheap simply does not exist in the loop. That contrast is the clearest evidence that choosing DP for the corridor was about the problem's shape, not about DP being a magic word. Working through this project and identifying the utility in Dynamic Programming myself is why Held-Karp was not included in the general prompt.</p><div class="table-scroll"><table>
<thead><tr><th></th><th>ROADTRIP (corridor mode)</th><th>OPTLOOP (loop mode)</th></tr></thead>
<tbody>
<tr><td>time</td><td>O(nights · n²)</td><td>O(2ⁿ · n²)</td></tr>
<tr><td>space</td><td>O(nights · n)</td><td>O(2ⁿ · n)</td></tr>
<tr><td>why</td><td>progress order replaces a visited set</td><td>no ordering — the state must remember visits</td></tr>
<tr><td>in practice</td><td>59 regions, instant</td><td>capped at 20 regions (0.25 GB; n=22 needs 1.1 GB)</td></tr>
</tbody></table></div><p class="fig-caption">One project, two subproblems, two complexity classes. The structure that makes one mode polynomial does not exist in the other.</p>`,
    },
    {
      heading: "What no longer remains in the final project",
      html: `<p>Two of the more sophisticated pieces of this project no longer exist, and removing them was part of its progress. The original project built length normalization so trips of different lengths could compete: pad the shorter route with average-quality phantom stops, then divide by the maximum. Once the number of nights was made an exact input, every route had the same length and there was nothing left to normalize, so for the sake of refraining from arbitrary and unnecessary math the calculation was removed. The ranked-alternatives table went the same way. Keeping clever code that is not entirely necessary would have been the same mistake as the hiking version, more minute arbitrary data to track, not highly justifiable, and it just confuses the code space of the algorithms I created. It made it harder to track whether my function was working properly in general, or just with the specific tests and datasets I was using.</p><blockquote class="prompt-block">before:&nbsp; finalScore = (rawSum + (MAX_STOPS - stops) * globalAvg) / MAX_STOPS<br>after:&nbsp;&nbsp; finalScore = rawSum / nights</blockquote>`,
    },
    {
      heading: "Known Limitations",
      html: `<p>Straight-line distance times 1.25 misses geography that forces detours, so some legs are optimistic. 1.25 is a magic number used as a placeholder for this current rendition of the project. This is OK for the scope of this project, as the main center of focus was the algorithms' efficiency as opposed to validity of routes. Something to factor into future renditions. The star ratings are my own judgment, not sourced data. The datasets are small enough that long trips exhaust them. And the random generator scatters regions across a box rather than along the Denver-to-San-Francisco corridor, leading to unsolvable trips. Each of these is a known simplification, not a surprise. The scope is to only use data that works for the program, but AI had filled in the gaps on the situations where routes cannot complete, which is fine, but also a learned waste of time.</p><p>The most interesting part of the project was working through the algorithms AI proposed and testing whether the implementations actually held up. Reviewing the code, I noticed that raising the mileage maximum would send the route hundreds of miles out of the way for almost nothing back. Moving --hi from 450 to 563, a ceiling chosen to match Claude #2's window, gained 0.01 in score and cost 672 miles. Because the score never falls as that ceiling rises, there is no point where the program stops on its own, and left unbounded it always takes the longer trip. I worked through several approaches with AI and evaluated whether each was worth implementing. Pareto frontiers to expose the score-versus-mileage tradeoff, parametric search to locate the exact threshold where a better route unlocks, detour ratios that weigh miles driven against progress made toward the destination, and a feasibility floor showing that no route can exist unless the daily maximum is at least the trip distance divided by the number of driving days. Automating that limit would remove arbitrary figures like the 450 used here, but it is out of scope for this study and belongs to the next phase of the project. The gap is another example of the deeper work a scalable project demands, work the generic prompts never surfaced. AI never flagged the problem until I went looking, and then it became the tool I used to explore the math behind a fix.</p>`,
    },
    {
      heading: "Takeaways",
      html: `<p>It seems that the user must know exactly how to code the problem in order to direct AI properly. AI served as a tool to collapse execution time, but not thinking time. The education is what bought the thinking, and the code that came back was only useful because I could read it, doubt it, and catch the one place where my own specification was wrong.</p><p>In this experiment, small wording changes consistently led different models toward different algorithmic formulations. In future projects, I do not believe I can simply avoid accidental phrases the AI interprets as a different type of project. Going into coding, I must refine the exact algorithms I plan to utilize so that AI can better understand the objectives I am trying to accomplish. My own project contains its own set of limitations and is still in progress. Editing that code with AI carries its own risk. Every suggestion is a chance for unrequested code to attach itself to the change, so each edit has to be read as carefully as the original. Prompts must be in a Goldilocks zone of brevity and specificity. This has been a study of the limitations of code writing for any AI usage, including in my own project. From DFS checking, I know my algorithm optimally and efficiently solved the exact specification at hand, a road trip destination planner given a specific set of input. It's harder to conclude that for all datasets, but it doesn't have to hold for all datasets.</p><p>Each of these AIs was able to recognize how to effectively use my algorithms and reinforced how such algorithms gain relevance as the dataset grows. The biggest lesson wasn't that AI produced inefficient code. It was that AI faithfully optimized the problem I described, not the one I imagined. Once I understood the algorithm myself, prompting stopped feeling like guesswork and became another programming language.</p><p>As a final comparison, here is what Google search’s AI returns me when I simply ask it to return to me a 7-day road trip to San Francisco from Denver:</p><figure class="shot">
  <div class="shot-frame">
    <div class="shot-bar">Google search AI</div>
    <img src="images/google-ai-prompt.png" width="920" height="190" loading="lazy" alt="The prompt given to Google search's AI: give me the best 7 stop roadtrip that finishes in san francisco on day 8 when travelling from Denver to San Francisco. nature preferred.">
    <img src="images/google-ai-itinerary.png" width="801" height="767" loading="lazy" alt="Google search AI's answer: an eight-day table of destination stops with driving distance and remaining distance to San Francisco — Grand Junction CO, Moab UT, Torrey UT, Bryce Canyon UT, Baker NV, South Lake Tahoe CA, Yosemite National Park CA, and San Francisco CA.">
  </div>
  <figcaption>The same question put to Google search's AI, and the itinerary it returns.</figcaption>
</figure><p>Some similarities, some differences. Still not specific enough data on where to spend the night, a motivating factor in including that level of detail in my code.</p>`,
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
          title: "ROADTRIP \u00b7 western-usa.txt (59 regions, defaults) \u00b7 road miles (gc \u00d7 1.25)",
          html: "<div class=\"table-scroll\"><table><thead><tr><th>stop</th><th>location</th><th>rating</th><th>leg (mi)</th><th>total (mi)</th></tr></thead><tbody><tr><td>1</td><td>RockyMountainNP</td><td>4.7</td><td>69.4</td><td>69.4</td></tr><tr><td>2</td><td>ArchesNP</td><td>4.7</td><td>295.1</td><td>364.5</td></tr><tr><td>3</td><td>GrandCanyonNP</td><td>4.9</td><td>289.9</td><td>654.5</td></tr><tr><td>4</td><td>BryceCanyonNP</td><td>4.8</td><td>132.9</td><td>787.4</td></tr><tr><td>5</td><td>ZionNP</td><td>4.9</td><td>62.9</td><td>850.3</td></tr><tr><td>6</td><td>SequoiaNP</td><td>4.7</td><td>388.9</td><td>1,239.3</td></tr><tr><td>7</td><td>YosemiteNP</td><td>4.9</td><td>136.6</td><td>1,375.9</td></tr><tr><td>dest</td><td>San Francisco</td><td>&mdash;</td><td>196.7</td><td>1,572.6</td></tr></tbody></table></div>",
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
      attribution: "AI rendition · Prompt #1",
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
          title: "Adapted to western-usa.txt \u00b7 its 1.18 road factor",
          html: "<div class=\"table-scroll\"><table><thead><tr><th>stop</th><th>location</th><th>rating</th><th>leg (mi)</th><th>total (mi)</th></tr></thead><tbody><tr><td>1</td><td>RockyMountainNP</td><td>4.7</td><td>66</td><td>66</td></tr><tr><td>2</td><td>ArchesNP</td><td>4.7</td><td>279</td><td>345</td></tr><tr><td>3</td><td>GrandCanyonNP</td><td>4.9</td><td>274</td><td>619</td></tr><tr><td>4</td><td>BryceCanyonNP</td><td>4.8</td><td>125</td><td>744</td></tr><tr><td>5</td><td>ZionNP</td><td>4.9</td><td>59</td><td>803</td></tr><tr><td>6</td><td>SequoiaNP</td><td>4.7</td><td>367</td><td>1,170</td></tr><tr><td>7</td><td>YosemiteNP</td><td>4.9</td><td>129</td><td>1,299</td></tr><tr><td>dest</td><td>San Francisco</td><td>&mdash;</td><td>not driven</td><td>1,299</td></tr></tbody></table></div>",
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
      attribution: "AI rendition · Prompt #1",
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
          title: "Adapted to western-usa.txt \u00b7 7 stops \u00b7 raw great-circle miles",
          html: "<div class=\"table-scroll\"><table><thead><tr><th>stop</th><th>location</th><th>rating</th><th>leg (mi)</th><th>total (mi)</th></tr></thead><tbody><tr><td>1</td><td>BlackCanyonNP</td><td>4.4</td><td>168</td><td>168</td></tr><tr><td>2</td><td>ArchesNP</td><td>4.7</td><td>100</td><td>268</td></tr><tr><td>3</td><td>CanyonlandsNP</td><td>4.6</td><td>32</td><td>300</td></tr><tr><td>4</td><td>CapitolReefNP</td><td>4.5</td><td>70</td><td>370</td></tr><tr><td>5</td><td>BryceCanyonNP</td><td>4.8</td><td>77</td><td>447</td></tr><tr><td>6</td><td>ZionNP</td><td>4.9</td><td>50</td><td>497</td></tr><tr><td>7</td><td>YosemiteNP</td><td>4.9</td><td>359</td><td>856</td></tr><tr><td>dest</td><td>San Francisco</td><td>&mdash;</td><td>157</td><td>1,014</td></tr></tbody></table></div>",
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
      attribution: "AI rendition · Prompt #1",
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
          title: "Westbound from Denver \u00b7 hardcoded data \u00b7 raw great-circle miles",
          html: "<div class=\"table-scroll\"><table><thead><tr><th>stop</th><th>location</th><th>rating</th><th>leg (mi)</th><th>total (mi)</th></tr></thead><tbody><tr><td>1</td><td>RockyMountainNP</td><td>4.7</td><td>55.5</td><td>55.5</td></tr><tr><td>2</td><td>ArchesNP</td><td>4.7</td><td>236.1</td><td>291.6</td></tr><tr><td>3</td><td>GrandCanyonNP</td><td>4.9</td><td>232.0</td><td>523.6</td></tr><tr><td>4</td><td>BryceCanyonNP</td><td>4.8</td><td>106.3</td><td>629.9</td></tr><tr><td>5</td><td>ZionNP</td><td>4.9</td><td>50.3</td><td>680.3</td></tr><tr><td>6</td><td>SequoiaNP</td><td>4.7</td><td>311.2</td><td>991.4</td></tr><tr><td>7</td><td>YosemiteNP</td><td>4.9</td><td>109.3</td><td>1,100.7</td></tr><tr><td>dest</td><td>San Francisco</td><td>&mdash;</td><td>not driven</td><td>1,100.7</td></tr></tbody></table></div>",
        },
      ],
      outputNote:
        "Same seven parks, mean rating 4.80 — with raw great-circle miles, so every leg understates real driving.",
    },

    "claude-code-2": {
      buildCmd: "g++ -std=c++17 -O2 -Iinclude -o road_trip src/main.cpp\n./road_trip \"Denver\" \"San Francisco\" 8 450",
      name: "Claude Code #2",
      attribution: "AI rendition · Prompt #2",
      signColor: "sign",
      summary:
        "A four-header project: geometry, a Location type, a CSV-backed location database with free-text name resolution, and a dp[k][i] optimizer over a progress-ordered DAG with parent-pointer reconstruction. The destination is a mandatory final node, the closing leg obeys the same 450 cap, and an impossible trip is refused with the feasibility arithmetic printed. The breadth is the flaw: it invented a 163-location gazetteer of the whole country, ratings included, and holds no opinion about mileage beyond a 1e-5 tie-break.",
      facts: [
        { k: "Search structure", v: "DP over (day, node), progress-ordered DAG" },
        { k: "Trip length", v: "user input 1\u201390 days; suggests floor + slack" },
        { k: "Feasibility", v: "refuses with \u201cneeds at least 3 day(s)\u201d arithmetic" },
        { k: "Data", v: "163-location invented US gazetteer (CSV)" },
        { k: "Distance", v: "raw great-circle; no road factor" },
        { k: "Mileage opinion", v: "none \u2014 1e-5 tie-break only" },
        { k: "Detour behavior", v: "Yellowstone: +575 mi for +0.1 rating" },
        { k: "Closing leg", v: "capped at 450 like every other day" },
      ],
      story: [
        "Run on the study data converted to its CSV schema, its 4-and-5-stop routes match my solver exactly, and at seven stops it detours through Yellowstone because nothing in its objective charges for distance. Asked for a 30-day trip on its own map, it returns a 30-stop, 4,848-mile scenic-collecting marathon \u2014 the exact outcome its own code comment says the day cap exists to prevent.",
        "It independently derived the feasibility floor this study derived for my own solver: direct distance over daily cap, refused with the arithmetic printed and a suggested default of floor plus slack.",
      ],
      outputs: [
        {
          title: "Denver \u2192 San Francisco \u00b7 study data via its CSV schema \u00b7 8-day request",
          html: "<div class=\"table-scroll\"><table><thead><tr><th>stop</th><th>location</th><th>rating</th><th>leg (mi)</th><th>total (mi)</th></tr></thead><tbody><tr><td>1</td><td>RockyMountainNP</td><td>4.7</td><td>55.5</td><td>55.5</td></tr><tr><td>2</td><td>YellowstoneNP</td><td>4.8</td><td>377.1</td><td>432.7</td></tr><tr><td>3</td><td>ArchesNP</td><td>4.7</td><td>396.8</td><td>829.5</td></tr><tr><td>4</td><td>GrandCanyonNP</td><td>4.9</td><td>232.0</td><td>1,061.4</td></tr><tr><td>5</td><td>BryceCanyonNP</td><td>4.8</td><td>106.3</td><td>1,167.8</td></tr><tr><td>6</td><td>ZionNP</td><td>4.9</td><td>50.3</td><td>1,218.1</td></tr><tr><td>7</td><td>YosemiteNP</td><td>4.9</td><td>358.6</td><td>1,576.8</td></tr><tr><td>dest</td><td>San Francisco</td><td>&mdash;</td><td>157.4</td><td>1,734.1</td></tr></tbody></table></div>",
        },
      ],
      outputNote:
        "Great-circle miles, no road factor \u2014 its 450 cap admits legs up to 562 road-equivalent, which is how the 433-mile Yellowstone leg is legal here and illegal in my project. Mean rating 4.81 over the seven scenic stops.",
    },

    "codex-2": {
      buildCmd: "g++ -std=c++17 -O2 -o road_trip_planner src/main.cpp\n./road_trip_planner western-usa.txt",
      name: "Codex #2",
      attribution: "AI rendition · Prompt #2",
      signColor: "sign",
      summary:
        "A genuine DP: candidates sorted by projected progress along the start-to-destination axis, each node relaxed against all predecessors, longest path over a DAG. Wrapped around it, a rating system of invented constants \u2014 rating \u00d7 80, category bonuses of 28 and 24, 0.055 per off-route mile, 0.10 per detour mile, a flat 7 per stop, cities cut to rating \u00d7 8. It accepts no trip length at all: fifteen stops is its own answer, and there is no way to disagree.",
      facts: [
        { k: "Search structure", v: "DP, longest path over progress-sorted DAG" },
        { k: "Trip length", v: "no input \u2014 self-chosen (15 for this trip)" },
        { k: "Scoring", v: "rating \u00d7 80 + bonus \u2212 penalties, constants unexplained" },
        { k: "Data", v: "reads the study file directly" },
        { k: "Data validation", v: "strictest of the four \u2014 rejects trailing content" },
        { k: "Distance", v: "great-circle with corridor filter (180 mi)" },
        { k: "Mean rating", v: "4.59 over its 15 stops" },
      ],
      story: [
        "The planner structure is sound \u2014 the same DAG relaxation my project uses. The concern is everything around it: constants with no stated origin that fit the selected data well, which is precisely what makes them hard to audit when outputs go wrong.",
        "It is also the only program in the study that validates its input file: appending anything after the declared 59 records makes it refuse to run.",
      ],
      outputs: [
        {
          title: "Denver \u2192 San Francisco \u00b7 western-usa data \u00b7 self-chosen 15 stops",
          html: "<div class=\"table-scroll\"><table><thead><tr><th>stop</th><th>location</th><th>rating</th><th>leg (mi)</th><th>total (mi)</th></tr></thead><tbody><tr><td>1</td><td>RockyMountainNP</td><td>4.7</td><td>66</td><td>66</td></tr><tr><td>2</td><td>GreatSandDunesNP</td><td>4.4</td><td>199</td><td>265</td></tr><tr><td>3</td><td>BlackCanyonNP</td><td>4.4</td><td>145</td><td>410</td></tr><tr><td>4</td><td>MesaVerdeNP</td><td>4.5</td><td>113</td><td>523</td></tr><tr><td>5</td><td>ArchesNP</td><td>4.7</td><td>136</td><td>659</td></tr><tr><td>6</td><td>CanyonlandsNP</td><td>4.6</td><td>38</td><td>697</td></tr><tr><td>7</td><td>MonumentValley</td><td>4.6</td><td>106</td><td>803</td></tr><tr><td>8</td><td>GoblinValleySP</td><td>4.3</td><td>127</td><td>930</td></tr><tr><td>9</td><td>CapitolReefNP</td><td>4.5</td><td>34</td><td>964</td></tr><tr><td>10</td><td>HorseshoeBend</td><td>4.4</td><td>118</td><td>1,082</td></tr><tr><td>11</td><td>BryceCanyonNP</td><td>4.8</td><td>80</td><td>1,162</td></tr><tr><td>12</td><td>ZionNP</td><td>4.9</td><td>59</td><td>1,221</td></tr><tr><td>13</td><td>DeathValleyNP</td><td>4.5</td><td>260</td><td>1,481</td></tr><tr><td>14</td><td>SequoiaNP</td><td>4.7</td><td>93</td><td>1,574</td></tr><tr><td>15</td><td>YosemiteNP</td><td>4.9</td><td>123</td><td>1,697</td></tr><tr><td>dest</td><td>San Francisco</td><td>&mdash;</td><td>178</td><td>1,876</td></tr></tbody></table></div>",
        },
      ],
      outputNote:
        "Great-circle miles as printed by the program. Fifteen stops in 1,876 miles, every leg under 450, mean rating 4.59 \u2014 lower than the 7-stop optima because breadth costs quality.",
    },

    "copilot-2": {
      buildCmd: "g++ -std=c++17 -O2 -o planner main.cpp\n./planner   # interactive: file name, then stop count",
      name: "CoPilot #2",
      attribution: "AI rendition · Prompt #2",
      signColor: "sign",
      summary:
        "One file, interactive prompts, and a faithful transcription of all 59 study sites hardcoded as an initializer list \u2014 with no file input at all. San Francisco was never a named site in the file, only the header coordinate, so the study trip cannot be requested. On Denver to Yosemite it self-chooses 18 stops, eleven of them towns, because its 0.2-per-hundred-miles penalty makes any town a net gain. Its pruning cutoff is not admissible, and solving its exact objective with a DP shows the cost: true optimum 187.8, its DFS returned 61.4.",
      facts: [
        { k: "Search structure", v: "DFS + backtracking, inadmissible pruning" },
        { k: "Trip length", v: "no input \u2014 self-chosen (18 incl. start)" },
        { k: "Data", v: "59 sites hardcoded, faithful to the digit; no file IO" },
        { k: "San Francisco", v: "not reachable \u2014 never a named site" },
        { k: "Route character", v: "11 of 18 stops are towns; mean 3.56" },
        { k: "Objective vs search", v: "true optimum 187.8; DFS returned 61.4" },
        { k: "Distance", v: "raw great-circle" },
      ],
      story: [
        "Because its strict forward rule makes the graph a DAG, its exact objective is solvable exactly by DP. That optimum is a 54-stop, 8,327-mile run scoring 187.8 \u2014 the objective rewards visiting nearly everything. Its own search returned 61.4. Two defects pointing in opposite directions produce output that only looks like a road trip.",
        "The banner says \u201cnatural places prioritized,\u201d but no term in the scoring actually prefers nature; towns are cheap, so the route stuffs itself with them.",
      ],
      outputs: [
        {
          title: "Denver \u2192 YosemiteNP \u00b7 hardcoded data \u00b7 self-chosen length",
          html: "<div class=\"table-scroll\"><table><thead><tr><th>stop</th><th>location</th><th>rating</th><th>leg (mi)</th><th>total (mi)</th></tr></thead><tbody><tr><td>1</td><td>Breckenridge,CO</td><td>3.8</td><td>58.6</td><td>58.6</td></tr><tr><td>2</td><td>Vail,CO</td><td>3.8</td><td>21.0</td><td>79.5</td></tr><tr><td>3</td><td>SteamboatSprings,CO</td><td>3.7</td><td>63.2</td><td>142.7</td></tr><tr><td>4</td><td>Aspen,CO</td><td>3.9</td><td>89.4</td><td>232.1</td></tr><tr><td>5</td><td>Telluride,CO</td><td>4.1</td><td>101.9</td><td>334.1</td></tr><tr><td>6</td><td>GrandJunction,CO</td><td>2.6</td><td>87.5</td><td>421.5</td></tr><tr><td>7</td><td>Moab,UT</td><td>4.0</td><td>63.6</td><td>485.1</td></tr><tr><td>8</td><td>GreenRiver,UT</td><td>2.9</td><td>43.9</td><td>529.0</td></tr><tr><td>9</td><td>Torrey,UT</td><td>3.6</td><td>83.3</td><td>612.3</td></tr><tr><td>10</td><td>Escalante,UT</td><td>3.6</td><td>37.9</td><td>650.2</td></tr><tr><td>11</td><td>Kanab,UT</td><td>3.6</td><td>71.2</td><td>721.4</td></tr><tr><td>12</td><td>Springdale,UT</td><td>4.0</td><td>27.8</td><td>749.2</td></tr><tr><td>13</td><td>StGeorge,UT</td><td>2.7</td><td>32.0</td><td>781.2</td></tr><tr><td>14</td><td>LasVegas,NV</td><td>2.4</td><td>108.0</td><td>889.3</td></tr><tr><td>15</td><td>Tonopah,NV</td><td>2.9</td><td>174.5</td><td>1,063.8</td></tr><tr><td>16</td><td>SequoiaNP</td><td>4.7</td><td>131.7</td><td>1,195.5</td></tr><tr><td>17</td><td>YosemiteNP</td><td>4.9</td><td>109.3</td><td>1,304.8</td></tr></tbody></table></div>",
        },
      ],
      outputNote:
        "Great-circle miles as printed. The route never drives to San Francisco because it cannot be asked to \u2014 Yosemite is the nearest destination its dataset can name. Mean 3.56 including the start and destination ratings its scorer counts.",
    },

  },

  /* ---------------- HEAD-TO-HEAD ---------------- */
  dataFile: "39.7392 -104.9903\n37.7749 -122.4194\n59\n39.7392 -104.9903 1\nDenver,CO 39.7392 -104.9903 2.8\n38.8339 -104.8214 1\nColoradoSprings,CO 38.8339 -104.8214 3.0\n39.4817 -106.0384 1\nBreckenridge,CO 39.4817 -106.0384 3.8\n39.6403 -106.3742 1\nVail,CO 39.6403 -106.3742 3.8\n40.485 -106.8317 1\nSteamboatSprings,CO 40.485 -106.8317 3.7\n39.1911 -106.8175 1\nAspen,CO 39.1911 -106.8175 3.9\n37.9375 -107.8123 1\nTelluride,CO 37.9375 -107.8123 4.1\n38.0228 -107.6714 1\nOuray,CO 38.0228 -107.6714 4.1\n37.8119 -107.6626 1\nSilverton,CO 37.8119 -107.6626 3.9\n37.2753 -107.8801 1\nDurango,CO 37.2753 -107.8801 3.7\n39.0639 -108.5506 1\nGrandJunction,CO 39.0639 -108.5506 2.6\n38.5733 -109.5498 1\nMoab,UT 38.5733 -109.5498 4.0\n38.9958 -110.1598 1\nGreenRiver,UT 38.9958 -110.1598 2.9\n38.2991 -111.4196 1\nTorrey,UT 38.2991 -111.4196 3.6\n37.7703 -111.6021 1\nEscalante,UT 37.7703 -111.6021 3.6\n37.0475 -112.5263 1\nKanab,UT 37.0475 -112.5263 3.6\n37.1889 -112.9986 1\nSpringdale,UT 37.1889 -112.9986 4.0\n37.0965 -113.5684 1\nStGeorge,UT 37.0965 -113.5684 2.7\n40.7608 -111.891 1\nSaltLakeCity,UT 40.7608 -111.891 2.5\n40.6461 -111.498 1\nParkCity,UT 40.6461 -111.498 3.5\n43.4799 -110.7624 1\nJacksonHole,WY 43.4799 -110.7624 4.1\n44.5263 -109.0565 1\nCody,WY 44.5263 -109.0565 3.5\n45.6796 -111.0386 1\nBozeman,MT 45.6796 -111.0386 3.1\n36.9147 -111.4558 1\nPage,AZ 36.9147 -111.4558 3.8\n35.1983 -111.6513 1\nFlagstaff,AZ 35.1983 -111.6513 2.7\n35.2495 -112.191 1\nWilliams,AZ 35.2495 -112.191 3.4\n34.8697 -111.7601 1\nSedona,AZ 34.8697 -111.7601 4.2\n34.7489 -112.1138 1\nJerome,AZ 34.7489 -112.1138 3.5\n34.54 -112.4685 1\nPrescott,AZ 34.54 -112.4685 3.3\n33.4484 -112.074 1\nPhoenix,AZ 33.4484 -112.074 2.4\n32.2226 -110.9747 1\nTucson,AZ 32.2226 -110.9747 2.6\n31.7118 -110.0678 1\nTombstone,AZ 31.7118 -110.0678 3.3\n31.4482 -109.9148 1\nBisbee,AZ 31.4482 -109.9148 3.4\n36.1716 -115.1398 1\nLasVegas,NV 36.1716 -115.1398 2.4\n35.9786 -114.8324 1\nBoulderCity,NV 35.9786 -114.8324 3.2\n39.2494 -114.8744 1\nEly,NV 39.2494 -114.8744 3.0\n38.0692 -117.2305 1\nTonopah,NV 38.0692 -117.2305 2.9\n34.1356 -116.0542 1\nTwentyninePalms,CA 34.1356 -116.0542 3.0\n33.8303 -116.5453 1\nPalmSprings,CA 33.8303 -116.5453 2.9\n38.5647 -110.714 1\nGoblinValleySP 38.5647 -110.714 4.3\n40.3428 -105.6836 1\nRockyMountainNP 40.3428 -105.6836 4.7\n37.7916 -105.5943 1\nGreatSandDunesNP 37.7916 -105.5943 4.4\n38.5754 -107.7416 1\nBlackCanyonNP 38.5754 -107.7416 4.4\n37.2402 -108.4613 1\nMesaVerdeNP 37.2402 -108.4613 4.5\n38.7331 -109.5925 1\nArchesNP 38.7331 -109.5925 4.7\n38.3269 -109.8783 1\nCanyonlandsNP 38.3269 -109.8783 4.6\n36.9914 -110.1939 1\nMonumentValley 36.9914 -110.1939 4.6\n38.367 -111.1742 1\nCapitolReefNP 38.367 -111.1742 4.5\n37.593 -112.1871 1\nBryceCanyonNP 37.593 -112.1871 4.8\n37.2982 -113.0263 1\nZionNP 37.2982 -113.0263 4.9\n36.8619 -111.3743 1\nHorseshoeBend 36.8619 -111.3743 4.4\n36.0544 -112.1401 1\nGrandCanyonNP 36.0544 -112.1401 4.9\n32.2967 -111.1666 1\nSaguaroNP 32.2967 -111.1666 4.3\n34.9099 -109.8068 1\nPetrifiedForestNP 34.9099 -109.8068 4.2\n44.428 -110.5885 1\nYellowstoneNP 44.428 -110.5885 4.8\n34.1341 -116.3131 1\nJoshuaTreeNP 34.1341 -116.3131 4.5\n37.8651 -119.5383 1\nYosemiteNP 37.8651 -119.5383 4.9\n36.4864 -118.5658 1\nSequoiaNP 36.4864 -118.5658 4.7\n36.5054 -117.0794 1\nDeathValleyNP 36.5054 -117.0794 4.5\n\n# NOTE: The numbers in the middle of this file (the count on each region\n# header line) are campgrounds. They are irrelevant to the current project\n# but will be used in a later phase.\n",

  comparison: {
        prompt2: {
          "title": "Prompt #2 · mean rating · total miles by scenic-stop count",
          "cols": [
                "stops",
                "My project",
                "Claude Code #2",
                "Codex #2",
                "CoPilot #2"
          ],
          "rows": [
                [
                      "1",
                      "no route",
                      "no route, floor printed",
                      "no length input",
                      "no length input"
                ],
                [
                      "2",
                      "4.80 · 1,229 mi",
                      "4.85 · 975 mi",
                      "no length input",
                      "no length input"
                ],
                [
                      "3",
                      "4.83 · 1,229 mi",
                      "4.87 · 982 mi",
                      "no length input",
                      "no length input"
                ],
                [
                      "4",
                      "4.85 · 1,379 mi",
                      "4.85 · 1,103 mi",
                      "no length input",
                      "no length input"
                ],
                [
                      "5",
                      "4.84 · 1,451 mi",
                      "4.84 · 1,160 mi",
                      "no length input",
                      "no length input"
                ],
                [
                      "6",
                      "4.82 · 1,528 mi",
                      "4.83 · 1,734 mi",
                      "no length input",
                      "no length input"
                ],
                [
                      "7",
                      "4.80 · 1,573 mi",
                      "4.81 · 1,734 mi",
                      "no length input",
                      "no length input"
                ],
                [
                      "15",
                      "4.63 · 2,587 mi",
                      "4.66 · 2,418 mi",
                      "4.59 · 1,876 mi (its pick)",
                      "n/a"
                ],
                [
                      "18",
                      "4.56 · 3,625 mi",
                      "4.59 · 2,424 mi (stops at 17)",
                      "n/a",
                      "3.56 · 1,305 mi (its pick, to Yosemite)"
                ]
          ],
          "note": "Mileage bases differ: my column is road miles (gc × 1.25); the prompt-2 programs report raw great-circle. Claude Code #2's day count includes the arrival day; rows align by scenic stops. CoPilot #2 cannot be asked for San Francisco, so its row drives Denver to Yosemite."
    },
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
