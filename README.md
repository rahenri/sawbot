# Sawbot

Contest:
https://docs.riddles.io/light-riders


## Approach summary

This contest was a tone of fun, but now that the contest is wrapping up, it
would be really nice if we could share the approaches that we used. I will
start with my approach.

First of all, I joined the contest relatively late, I only started working on
it on the last month and didn't have much time on the month anyway because of
travels, being busy at work, etc.

My bot is largely the code I used for ultimate tic-tac-toe adapted to a
different game. I did use an minimax search despite it not being a perfect fit,
because in the game, the players move simultaneously, whereas, minimax is
applied on games where the players move one at a time. So my bot was actually
playing a slightly different game where it would choose a move first, and the
opponent would choose a move next knowing what is the move that my bot
performed. It was generally a good approximation, I think there weren't many
situations that knowing the opponent move represented a significant advantage,
however, I didn't really investigate that in detail. As soon as my bot was
doing reasonably well, I accepted that was a fair approximation.

In the end, I didn't have time to port all features from my tic-tac-toe bot to
this one, but here is the list of what I did:

* Alpha-beta pruning
* Move sorting (try moves hugging the wall first)
* Transposition table
* Evaluation function: I used a combination of area nearest to my bot, and
  edges on that area. I added a penalty for crossing articulations so the bot
  wouldn't count too much on the space beyond articulations. I tried the
  chamber heuristic found at [a1k0n postmortem], but without success. I did
  incorporate some ideas from there though. At the end I trained a NN combining
  a few features such as area, edges, chamber area (reachable space without
  crossing articulation), whether a bot can reach the other one, etc. This is a
  little better than my handcrafted scoring function (about 80 ELO points
  more).

An interesting thing that I found out during the contest is that, after some
point, making the bot faster was helping very little. Using the transposition
table to augment move sorting with iterative deepening made the bot much
faster, ie, it could reach another 8 or 10 moves in search depth, however, the
improvement was marginal. It was clear that investing on the evaluation
function would be more fruitful.

In the last week, I also tried training a NN where the input was the raw board
instead of handcrafted features. However, I wasn't even able to beat the random
bot with that approach, I certainly needed more time and more computation power
to make any progress there. Or my NN was just terrible =).

With that said, I believe that MCTS might have been a good choise for this
contest. I tried in the hack-man contest but did poorly and my initial instinct
was not to try it again =).

[a1k0n postmortem] https://www.a1k0n.net/2010/03/04/google-ai-postmortem.html
