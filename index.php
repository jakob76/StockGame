<?php
session_start();

if (!isset($_POST['load']) && !isset($_POST['exit']) && 
    !isset($_POST['lookup']) && !isset($_POST['buy']) && !isset($_POST['sell']) && 
    !isset($_POST['save_file_name'])) {
    session_unset();
    session_destroy();
    session_start(); // restart fresh session
}

$portfolio = $_SESSION['portfolio'] ?? [];
$cash = $_SESSION['cash'] ?? 5000.0;
$saveFile = $_SESSION['saveFile'] ?? '';

function run_backend($action, $ticker = '', $shares = 0) {
    $ticker = strtoupper(trim($ticker));
    $cmd = escapeshellcmd("./stock_game_backend $action $ticker $shares");
    $output = shell_exec($cmd);
    return !empty($output) ? json_decode($output, true) : ["error" => "No output from backend."];
}

// Load save file
if (isset($_POST['load'])) {
    $filename = $_POST['load_file'];
    if (file_exists($filename)) {
        $data = json_decode(file_get_contents($filename), true);
        $_SESSION['cash'] = $cash = $data['cash'];
        $_SESSION['portfolio'] = $portfolio = $data['portfolio'];
        $_SESSION['saveFile'] = $saveFile = $filename;
        $message = "Loaded from $filename";
    } else {
        $message = "Save file not found.";
    }
}

if (isset($_POST['exit'])) {
    $filename = trim($_POST['save_file_name'] ?? '');

    if ($filename === '') {
        $filename = $saveFile !== '' ? $saveFile : 'portfolio_save.json';
    }

    $_SESSION['saveFile'] = $saveFile = $filename;
    $data = ['cash' => $cash, 'portfolio' => $portfolio];
    file_put_contents($filename, json_encode($data, JSON_PRETTY_PRINT));
    session_destroy();
    echo "<script>alert('Portfolio saved to $filename. Game exited.'); window.location.href = window.location.pathname;</script>";
    exit;
}

// Set the save file name
if (isset($_POST['save_file_name'])) {
    $filename = trim($_POST['save_file_name']);

    // Use loaded filename if input is blank
    if ($filename === '' && isset($_SESSION['saveFile'])) {
        $filename = $_SESSION['saveFile'];
    }

    if ($filename !== '') {
        $_SESSION['saveFile'] = $saveFile = $filename;
        $data = ['cash' => $cash, 'portfolio' => $portfolio];
        file_put_contents($filename, json_encode($data, JSON_PRETTY_PRINT));
        session_destroy();
        echo "<script>alert('Portfolio saved to $filename. Game exited.'); window.location.href = window.location.pathname;</script>";
        exit;
    } else {
        $message = "Please enter a valid file name.";
    }
}
// Stock price lookup
if (isset($_POST['lookup'])) {
    $ticker = $_POST['ticker'];
    $result = run_backend("price", $ticker);
}

// Buy stocks
if (isset($_POST['buy'])) {
    $ticker = $_POST['ticker'];
    $shares = intval($_POST['shares']);
    $info = run_backend("price", $ticker);
    if (!isset($info['error']) && $info['price'] > 0) {
        $cost = $info['price'] * $shares;
        if ($cost <= $cash) {
            $cash -= $cost;
            if (isset($portfolio[$ticker])) {
                $oldShares = $portfolio[$ticker][0];
                $oldAvg = $portfolio[$ticker][1];
                $newShares = $oldShares + $shares;
                $newAvg = (($oldShares * $oldAvg) + $cost) / $newShares;
                $portfolio[$ticker] = [$newShares, $newAvg];
            } else {
                $portfolio[$ticker] = [$shares, $info['price']];
            }
            $_SESSION['cash'] = $cash;
            $_SESSION['portfolio'] = $portfolio;
            $message = "Bought $shares shares of $ticker.";
        } else {
            $message = "Insufficient cash.";
        }
    } else {
        $message = $info['error'] ?? "Failed to fetch price.";
    }
}

// Sell stocks
if (isset($_POST['sell'])) {
    $ticker = $_POST['ticker'];
    $shares = intval($_POST['shares']);
    if (!isset($portfolio[$ticker])) {
        $message = "You don't own $ticker.";
    } elseif ($shares > $portfolio[$ticker][0]) {
        $message = "Not enough shares to sell.";
    } else {
        $info = run_backend("price", $ticker);
        if (!isset($info['error']) && $info['price'] > 0) {
            $revenue = $info['price'] * $shares;
            $cash += $revenue;
            $portfolio[$ticker][0] -= $shares;
            if ($portfolio[$ticker][0] === 0) unset($portfolio[$ticker]);
            $_SESSION['cash'] = $cash;
            $_SESSION['portfolio'] = $portfolio;
            $message = "Sold $shares shares of $ticker.";
        } else {
            $message = $info['error'] ?? "Failed to fetch price.";
        }
    }
}
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Stock Market Game</title>
    <style>
        body {
            font-family: 'Arial', sans-serif;
            background-color: #f4f4f9;
            margin: 0;
            padding: 0;
            color: #333;
        }
        header {
            background-color: #0052cc;
            color: white;
            padding: 15px;
            text-align: center;
        }
        h1 {
            margin: 0;
        }
        .container {
            width: 80%;
            margin: 30px auto;
        }
        .form-container {
            background-color: #fff;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
        }
        .form-container input, .form-container button {
            padding: 10px;
            margin: 5px;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        .form-container button {
            background-color: #28a745;
            color: white;
            cursor: pointer;
        }
        .form-container button:hover {
            background-color: #218838;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 20px;
        }
        th, td {
            padding: 10px;
            text-align: center;
            border: 1px solid #ddd;
        }
        th {
            background-color: #f1f1f1;
        }
        .message {
            padding: 10px;
            background-color: #f8d7da;
            color: #721c24;
            border-radius: 5px;
            margin-bottom: 20px;
        }
        .success {
            background-color: #d4edda;
            color: #155724;
        }
    </style>
</head>
<body>

<header>
    <h1>Welcome to the Stock Market Game</h1>
    <p>Trade, manage your portfolio, and become a stock market expert!</p>
</header>

<div class="container">

<?php if (!empty($message)): ?>
    <div class="message <?= strpos($message, 'Error') === false ? 'success' : '' ?>">
        <strong><?= $message ?></strong>
    </div>
<?php endif; ?>

<?php if (isset($result) && !isset($result['error'])): ?>
    <div class="message">
        <p><strong>Ticker:</strong> <?= htmlspecialchars($result['ticker']) ?></p>
        <p><strong>Price:</strong> $<?= number_format($result['price'], 2) ?></p>
        <p><strong>Date:</strong> <?= htmlspecialchars($result['date']) ?></p>
    </div>
<?php elseif (isset($result['error'])): ?>
    <div class="message">
        <p style="color:red;">Error: <?= htmlspecialchars($result['error']) ?></p>
    </div>
<?php endif; ?>

<div class="form-container">
    <h2>Load Save File</h2>
    <form method="POST">
        <input type="text" name="load_file" placeholder="filename.json">
        <button name="load">Load</button>
    </form>
</div>

<div class="form-container">
    <h2>Lookup Stock Price</h2>
    <form method="POST">
        <input type="text" name="ticker" required placeholder="Stock Ticker">
        <button name="lookup">Lookup Price</button>
    </form>
</div>

<div class="form-container">
    <h2>Buy Stocks</h2>
    <form method="POST">
        <input type="text" name="ticker" required placeholder="Stock Ticker">
        <input type="number" name="shares" required placeholder="Shares to Buy">
        <button name="buy">Buy</button>
    </form>
</div>

<div class="form-container">
    <h2>Sell Stocks</h2>
    <form method="POST">
        <input type="text" name="ticker" required placeholder="Stock Ticker">
        <input type="number" name="shares" required placeholder="Shares to Sell">
        <button name="sell">Sell</button>
    </form>
</div>

<h2>Your Portfolio</h2>
<p><strong>Cash:</strong> $<?= number_format($cash, 2) ?></p>

<?php
$totalNetWorth = $cash;
$portfolio = $_SESSION['portfolio'] ?? [];

foreach ($portfolio as $ticker => $data) {
    if (!isset($data[0]) || $data[0] <= 0) {
        continue; // skip stocks with no shares
    }

    $info = run_backend("price", $ticker);

    if (isset($info['price']) && $info['price'] > 0) {
        $currentPrice = $info['price'];
        $totalValue = $data[0] * $currentPrice;
        $totalNetWorth += $totalValue;
    } else {
        echo "<p>Error: Couldn't fetch price for " . htmlspecialchars($ticker) . ".</p>";
    }
}


?>
<p><strong>Total Net Worth:</strong> $<?= number_format($totalNetWorth, 2) ?></p>

<?php if (empty($portfolio)): ?>
    <p>You don't own any stocks yet. Time to make some moves!</p>
<?php else: ?>
<table>
    <tr>
        <th>Ticker</th>
        <th>Shares</th>
        <th>Avg. Price</th>
        <th>Total Value (Current)</th>
    </tr>
    <?php foreach ($portfolio as $ticker => $data): ?>
        <?php
            if ($data[0] <= 0) continue;  // Skip if no shares
            $shares = $data[0];
            $avgPrice = $data[1];
            $info = run_backend("price", $ticker);
            $currentPrice = isset($info['price']) ? $info['price'] : 0;
            $totalValue = $shares * $currentPrice;
        ?>
        <tr>
            <td><?= htmlspecialchars($ticker) ?></td>
            <td><?= $shares ?></td>
            <td>$<?= number_format($avgPrice, 2) ?></td>
            <td>$<?= number_format($totalValue, 2) ?></td>
        </tr>
    <?php endforeach; ?>
</table>
<?php endif; ?>


<div class="form-container" style="max-width: 400px; margin-top: 20px;">
    <h2 style="font-size: 1.2em;">Save and Exit</h2>
    <form method="POST">
        <input type="text" name="save_file_name" id="save_file_name"
            placeholder="portfolio_save.json"
            value="<?= htmlspecialchars($saveFile ?: '') ?>"
            style="width: 94%; margin-bottom: 10px;">
        <button name="exit" style="width: 100%;">Save and Exit</button>
    </form>
</div>

</div>

</body>
</html>
