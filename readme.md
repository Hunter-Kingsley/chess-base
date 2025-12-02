# Fen Strings
I implemented a simple algorithm to parse the FEN string by looping over it and either going a certain amount of squares forward, going to the next line, or placing the correct piece down depending on the character within the string

# Knight, Pawn, and King Movement
I used bitboards to create and track the possible moves for these pieces (as I will do with future pieces). The King and Knight use identical logic. I have an array to store the possible moves for each piece from all 64 squares and functions for each piece that add that piece's move offset and add all those possible move bitboards into the moves list
![BaseChess](BaseChess.png)

The pawns are less straightforward because black and white pawns do not move the same way from the same squares. To create the correct moves for each pawn, I have to create a total of 8 different bitboards (one for single moves, one for double moves based on if they did a single move from the starting position, one for both left and right captures, and then do all that twice for black and white pawns). After creating all those bitboards, I can take the shift needed to go from the pawn's starting position to all the possible places it's allowed to go and add those to the move list
![First20Moves](First20KnightMoves.png)

# Post AI with Negamax
My AI was successfully able to reach a depth of 6, which I think is fairly respectable. The way the AI plays is very amusing to me because it is so focused on capturing material that it can actually catch you off guard with aggressive moves that can screw you over if you don't have the matchup experience to easily counter it. For as good as it is on offense, it is horrible on defense. I beat it with the queen's gambit every time, hahaha. The most annoying portion of this phase of the assignment for me was implementing the castling and promotion of the pieces. I really tried to find a way that I could reuse the bit flags pattern on the player moves the same way the AI can, but everything I tried ended up being way too convoluted, so I ended up just checking if the specific conditions had been met within the bitMovedFromTo() function. Before Friday, I'll try and look into changing the opening moves a bit to see if it is capable of a more traditional opening.

https://github.com/user-attachments/assets/ba4fa3a1-579f-4056-9b31-febb17756c20

