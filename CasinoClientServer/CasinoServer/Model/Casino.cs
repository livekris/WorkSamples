using LiteDB;

namespace Model
{
    public record Player
    {
        [BsonId]
        public string Name { get; set; } = default!; // name of the player
        public static readonly int InitialBalance = 10; // each player starts with 10 credits but we also allow custom input
        public int Balance { get; private set; } = InitialBalance; // the player balance 

        public void Credit(int amount) => Balance += amount;
        public void Debit(int amount) => Balance -= amount;

        public void Cashout() => Balance = 0;
    }

    // Players will be stored in a database.
    // Even though this is a simple app, we want to handle cases where the server goes offline.
    // This ensures that players’ money is tracked reliably, demonstrating our commitment to honoring
    // contracts with clients. If the server goes down mid-play, the client can still recover their
    // money in some form, since it is stored in a database
    public class PlayerDatabase : IDisposable
    {
        private readonly LiteDatabase Database; // Players database
        private bool DisposedFlag = false;

        // Helps us get players and updating etc.
        private ILiteCollection<Player> Collection => Database.GetCollection<Player>("players");
        public PlayerDatabase(string dbPath = "players.db")
        {
            Database = new LiteDatabase(dbPath);
        }
        public Player? GetPlayer(string name) => Collection.FindById(name);

        public void Update(Player player) => Collection.Upsert(player);

        public bool Remove(string name) => Collection.Delete(name);

        // IDisposable implementation (we are not deriving from this class so keeping it simple)
        public void Dispose()
        {
            if (!DisposedFlag)
            {
                Database.Dispose();      // Dispose the LiteDatabase instance
                DisposedFlag = true;
            }
        }
    }

    // Implements the slot machine logic behind the scenes
    public class SlotMachine
    {
        private static readonly Random random = new(); 

        private static readonly int FruitCount = 4; 

        public static int Won(int[] slots)
        {
            int multiple = 10;
            return slots.Distinct().Count() switch
            {
                1 => (slots[0] + 1) * multiple,   // all three match
                2 => 0,                           // any two match
                _ => 0
            };
        }

        // "Rolls" reels and determines winnings 
        public static int[] Roll(out int winnings)
        {
            int[] slots = Enumerable.Range(0, FruitCount - 1)
                                    .Select(_ => random.Next(0, FruitCount))
                                    .ToArray();

            winnings = Won(slots);
            return slots;
        }

        // Plays the slots with the cheats
        public static int[] PlaySlots(Player player, out int winnings)
        {
            int[] slots = Roll(out winnings);
            int balance = player.Balance;

            // Cheat rules: if the player won, roll again to cheat,
            //  but with 30% chance if winnings are between 40 and 60 
            if (winnings > 0 && balance >= 40 && balance <= 60 && random.NextDouble() <= 0.3)
            {
                slots = Roll(out winnings); 
            }
            else if (winnings > 0 && balance > 60 && random.NextDouble() <= 0.6)
            {
                slots = Roll(out winnings);
            }

            return slots;
        }
    }

}