using Microsoft.Maui.Controls;
using System;
using System.Net.Http;
using System.Net.Http.Json;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Security.Cryptography;
using System.Text;

namespace CasinoClientUI;

public partial class MainPage : ContentPage
{
	private static readonly HttpClient client = new HttpClient
	{
		BaseAddress = new Uri("http://127.0.0.1:5000") // must match server port
	};
	private static readonly int SlotCount = 3; // for 3 slots
	public int[] Slots { get; private set; } = new int[SlotCount];
	private string PlayerName = "Alice"; // name is not unique but this was not specified in the excercise
	private static int Balance = 0;
	private static readonly int ReelDelayTime = 1000; // in milliseconds; each reel is delayed by 1,2,3 seconds
	private static readonly uint RotationDelayTime = 100; // revolving X delay 
	private Random random = new Random();
	public MainPage()
	{
		InitializeComponent();
	}

	// Enter button is the entry point to the slot machine (a name must be provided)
	// The name is used to store the data incoming from the client (bets/winnings) in a database
	private async void EnterButton_Clicked(object sender, EventArgs e)
	{
		string result = await DisplayPromptAsync(
			"Enter Name",
			"Please enter your player name:",
			"OK",
			"Cancel",
			placeholder: "Name");

		if (!string.IsNullOrWhiteSpace(result))
		{
			PlayerName = result;
			PlayerNameLabel.Text = $"Player: {PlayerName}";

			_ = MainThread.InvokeOnMainThreadAsync(async () =>
			{
				_ = EnterCasino(); // register player on startup
			});
		}
		else
		{
			await DisplayAlert("Error", "Player name is required.", "OK");
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// Player enters casino
	//
	private async Task EnterCasino()
	{
		SetBalance(0);
		SetButtonsEnabled(true);

		CasinoContainer.IsVisible = true;
		CashOutButton.IsVisible = true;
		
		if (string.IsNullOrWhiteSpace(PlayerName))
		{
			await DisplayAlert("Error", "Player has to have a name", "OK");
			return;
		}

		// Call PingServer with the specific action.
		(bool success, var response) = await HttpActionUI(async () =>
		await client.PostAsync($"/players/enter?name={PlayerName}", null), "Failed to enter casino: ");

		if (success)
			await GetBalance(); // retrieves credits from the casino

		if (Balance == 0)
			SetButtonsEnabled(false); // disable buttons as we have no more credits
	}

	// Player is notified of their balance
	private async Task GetBalance()
	{
		// Call PingServer with the specific action.
		(bool success, var response) = await HttpActionUI(async () =>
		await client.GetAsync($"/players/balance?name={PlayerName}"), "Failed to get balance: ");

		// get balance from the server and update the label
		var balance = await response.Content.ReadFromJsonAsync<int>();
		if (balance != null)
			SetBalance(balance);
	}

	// Sets local balance and client balance label
	private void SetBalance(int balance)
	{
		Balance = balance;
		BalanceLabel.Text = $"Balance: ${balance}";
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// Player leaves casino
	//
	// Safely terminate the application if the user closes the window
	protected override void OnDisappearing()
	{
		_ = TryLeaveCasino();
		base.OnDisappearing();
	}

	// Attempt to leave and throw an error if the act is problematic
	private async Task TryLeaveCasino()
	{
		try
		{
			await client.PostAsync($"/players/leave?name={PlayerName}", null);
		}
		catch (Exception ex)
		{
			System.Diagnostics.Debug.WriteLine($"Error leaving casino: {ex.Message}");
		}
	}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Player cashes out
	//
	private async void CashOutButton_Clicked(object sender, EventArgs e)
	{
		await CashOut();
		SetButtonsEnabled(false);
	}

	private void OnCashOutButtonPointerEntered(object sender, EventArgs e)
	{
		double roll = random.NextDouble();
		double moveChance = 0.5;

		if (roll < moveChance)
		{
			MoveButtonRandomly(); // Move the button randomly when hovered
		}
	}

	private void OnCashOutButtonPointerExited(object sender, EventArgs e)
	{
		// do nothing; we need this
	}
	private void MoveButtonRandomly()
	{
		// Pick random direction and move exactly 300 pixels to the new spot
		double distance = 300; // fixed distance
		double angle = random.NextDouble() * 2 * Math.PI; // random angle

		// Calculate deltaX and deltaY using cosine and sine
		double deltaX = distance * Math.Cos(angle);
		double deltaY = distance * Math.Sin(angle);

		CashOutButton.TranslationX += deltaX;
		CashOutButton.TranslationY += deltaY;
	}

	// Could not get hover working on MAUI so implemented Pressed instead to do what hover does
	private void CashOutButton_Pressed(object sender, EventArgs e)
	{
		double roll = random.NextDouble();
		double unclickableChance = 0.4;

		if (roll < unclickableChance)
		{
			CashOutButton.IsEnabled = false;
			Device.StartTimer(TimeSpan.FromSeconds(1), () =>
			{
				CashOutButton.IsEnabled = true;
				return false;
			});
		}
	}

	// Enables user to cashout
	private async Task CashOut()
	{
		// Call Post/Get with the specific action.
		(bool success, var response) = await HttpActionUI(async () =>
		await client.PostAsync($"/players/cashout?name={PlayerName}", null), "Failed to cash out: ");

		if (!success)
			return;

		var cashout = await response.Content.ReadFromJsonAsync<int>();
		if (cashout != null)
		{
			SetBalance(0);
			await DisplayAlert("CASH OUT", $"You cashed out ${cashout}", "OK");
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// Player plays slots
	//
	private async void PlayButton_Clicked(object sender, EventArgs e)
	{
		await PlaySlotMachine();
	}
	private async Task PlaySlotMachine()
	{
		// Call Post/Get with the specific action.
		(bool success, var response) = await HttpActionUI(async () =>
		await client.PostAsync($"/players/play?name={PlayerName}", null), "Failed to play slots: ");

		if (!success)
		{
			PingServer(); // this shall output to the console some information about the server health
			return; // ideally we might want to do some more 
		}

		var result = await response.Content.ReadFromJsonAsync<SlotResult>();
		if (result != null)
		{
			Slots = result.Slots; // 

			SetButtonsEnabled(false);

			Label[] slotLabels = new Label[] { Slot1Label, Slot2Label, Slot3Label };
			StartSpinning(slotLabels);

			await RevealSlot(0, Slot1Label, Slot1, Slots[0], ReelDelayTime);
			await RevealSlot(1, Slot2Label, Slot2, Slots[1], ReelDelayTime);
			await RevealSlot(2, Slot3Label, Slot3, Slots[2], ReelDelayTime);

			await Task.Delay(500); // slight delay for effect, before we update the balance 
								   // Reveal balance at the end

			// set new balance after a potential win
			SetBalance(result.Balance);

			if (Balance > 0)
				SetButtonsEnabled(true);
		}
	}

	private void SetButtonsEnabled(bool enabled)
	{
		PlayButton.IsEnabled = enabled;
		CashOutButton.IsEnabled = enabled;

		PlayButton.BackgroundColor = (enabled ? Colors.DarkBlue : Colors.LightGray);
		CashOutButton.BackgroundColor = (enabled ? Colors.Green : Colors.LightGray);

		PlayButton.TextColor = (enabled ? Colors.White : Colors.DarkGray);
		CashOutButton.TextColor = (enabled ? Colors.White : Colors.DarkGray);

		// reset translation of the cashout button in case it had been moved
		CashOutButton.TranslationX = 0;
		CashOutButton.TranslationY = 0;
	}
	
	// This function pings the serve to find out if the server is up or down 
	// (more for debugging but also waiting on the server whenever the user attempts to play)
	public static async Task<bool> PingServer(int retryDelayMs = 1000)
	{
		while (true)
		{
			try
			{
				var response = await client.GetAsync("/health"); // lightweight ping
				if (response.IsSuccessStatusCode)
				{
					Console.WriteLine("Server is up!");
					return true;
				}
			}
			catch (HttpRequestException)
			{
				Console.WriteLine("Server is down, retrying...");
			}

			await Task.Delay(retryDelayMs); // wait before retrying
		}
	}

	private (string Symbol, Color Color) GetSlotInfo(int value) => value switch
	{
		//C=🍒  L=🍋  O=🍊  W=🍉
		0 => ("🍒", Colors.Red), // Cherry
		1 => ("🍋", Colors.Yellow), // Lemon
		2 => ("🍊", Colors.Orange), // Orange
		3 => ("🍉", Colors.Green), // Watermelon
		_ => ("?", Colors.Gray)
	};

	private void UpdateSlot(Label label, BoxView box, int value)
	{
		var (symbol, color) = GetSlotInfo(value);
		label.Text = symbol;
		box.Color = color;
	}

	//Array to store cancellation tokens for each slot
	private readonly CancellationTokenSource[] cts = new CancellationTokenSource[SlotCount];

	private void StartSpinning(Label[] slotLabels)
	{
		for (int i = 0; i < slotLabels.Length; i++)
		{
			// Rotating Xs on each slot
			slotLabels[i].Text = "X";

			// Create a CancellationTokenSource for this slot
			cts[i] = new CancellationTokenSource();
			var token = cts[i].Token;
			var label = slotLabels[i];
			
			// Start spinning asynchronously
			_ = MainThread.InvokeOnMainThreadAsync(async () =>
			{
				// Initial rotation
				await label.RelRotateTo(360, RotationDelayTime);

				while (!token.IsCancellationRequested)
				{
					await label.RelRotateTo(360, RotationDelayTime, Easing.Linear);
				}
			});
		}
	}

	// Stop spinning a slot as input by index
	private void StopSpinning(int index)
	{
		if (cts[index] != null && !cts[index].IsCancellationRequested)
		{
			cts[index].Cancel();
		}
		cts[index]?.Dispose();
		cts[index] = null;
	}
	
	// Reveal each reel one by one
	private async Task RevealSlot(int index, Label label, BoxView box, int value, int delay)
	{
		await Task.Delay(delay);
		await MainThread.InvokeOnMainThreadAsync(() =>
		{
			StopSpinning(index);
			UpdateSlot(label, box, value);
		});
	}

	// generalize http actions since this pattern will be followed a few times
	// we are keeping the model and view in one function here as it is a small app
	private async Task<(bool status, HttpResponseMessage? Response)> HttpActionUI(
	Func<Task<HttpResponseMessage>> httpAction, string? customMessage = null)
	{
		try
		{
			var response = await httpAction();

			if (!response.IsSuccessStatusCode)
			{
				// use the custom message or none if not provided
				if (customMessage != null)
				{
					string errorMessage = $"{customMessage} ({response.StatusCode})";
					await DisplayAlert("Error", errorMessage, "OK");
				}
				return (false, null);
			}

			return (true, response);
		}
		catch (Exception ex)
		{
			await DisplayAlert("Error", $"Exception: {ex.Message}", "OK");

			return (false, null);
		}
	}

	public class SlotResult
	{
		public int[] Slots { get; set; } = new int[SlotCount];
		public int Balance { get; set; }
	}
}

