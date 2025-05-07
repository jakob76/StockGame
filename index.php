<?php
session_start();

if (!isset($_POST['load']) && !isset($_POST['exit']) && 
    !isset($_POST['lookup']) && !isset($_POST['buy']) && !isset($_POST['sell']) && 
    !isset($_POST['save_file_name'])) {
    session_unset();
    session_destroy();
    session_start();
}

$portfolio = $_SESSION['portfolio'] ?? [];
$cash = $_SESSION['cash'] ?? 5000.0;
$saveFile = $_SESSION['saveFile'] ?? '';

function run_backend($action, $ticker = '', $shares = 0) {
    $ticker = strtoupper(trim($ticker));
    if (isset($_SESSION['cached_prices'][$ticker])) {
        return $_SESSION['cached_prices'][$ticker];
    }
    $cmd = escapeshellcmd("./stock_game_backend $action $ticker $shares");
    $output = shell_exec($cmd);
    if (empty($output)) {
        return ["error" => "No output from backend."];
    }
    file_put_contents("backend_debug_log.txt", "CMD: $cmd\nOUTPUT: $output\n\n", FILE_APPEND);
    $data = json_decode($output, true);
    if (isset($data['price'])) {
        $_SESSION['cached_prices'][$ticker] = $data;
    }
    return $data ?: ["error" => "Failed to decode JSON output."];
}

function update_total_value($ticker, $shares, $prevPrice) {
    $info = run_backend("price", $ticker);
    if (!isset($info['error']) && $info['price'] > 0) {
        $currentPrice = $info['price'];
        $totalValue = $shares * $currentPrice;
        return [$shares, $prevPrice, $currentPrice, $totalValue];
    }
    return [$shares, $prevPrice, 0, 0];
}

if (isset($_POST['load'])) {
    $filename = $_POST['load_file'];
    if (file_exists($filename)) {
        $data = json_decode(file_get_contents($filename), true);
        $_SESSION['cash'] = $cash = $data['cash'];
        $_SESSION['portfolio'] = $data['portfolio'];
        $_SESSION['saveFile'] = $saveFile = $filename;

        foreach ($_SESSION['portfolio'] as $ticker => $stockData) {
            list($shares, $prevPrice) = $stockData;
            $info = run_backend("price", $ticker);
            if (!isset($info['error']) && $info['price'] > 0) {
                $currentPrice = $info['price'];
                $totalValue = $shares * $currentPrice;
                $_SESSION['portfolio'][$ticker] = [$shares, $prevPrice, $currentPrice, $totalValue];
            } else {
                $_SESSION['portfolio'][$ticker] = [$shares, $prevPrice, 0, 0];
            }
        }

        $portfolio = $_SESSION['portfolio'];
        $message = "Loaded and updated portfolio from $filename.";
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

if (isset($_POST['save_file_name'])) {
    $filename = trim($_POST['save_file_name']);
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

if (isset($_POST['lookup'])) {
    $ticker = $_POST['ticker'];
    $result = run_backend("price", $ticker);
}

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
                $prevPrice = $portfolio[$ticker][1];
                $newShares = $oldShares + $shares;
                $newPrev = (($oldShares * $prevPrice) + $cost) / $newShares;
                list($newShares, $newPrev, $currPrice, $totalValue) = update_total_value($ticker, $newShares, $newPrev);
                $portfolio[$ticker] = [$newShares, $newPrev, $currPrice, $totalValue];
            } else {
                list($shares, $newPrev, $currPrice, $totalValue) = update_total_value($ticker, $shares, $info['price']);
                $portfolio[$ticker] = [$shares, $newPrev, $currPrice, $totalValue];
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
            if (isset($portfolio[$ticker])) {
                list($remainingShares, $prevPrice, $currPrice, $totalValue) = update_total_value($ticker, $portfolio[$ticker][0], $portfolio[$ticker][1]);
                $portfolio[$ticker] = [$remainingShares, $prevPrice, $currPrice, $totalValue];
            }
            $_SESSION['cash'] = $cash;
            $_SESSION['portfolio'] = $portfolio;
            $message = "Sold $shares shares of $ticker.";
        } else {
            $message = $info['error'] ?? "Failed to fetch price.";
        }
    }
}

$totalStockValue = 0;
foreach ($portfolio as $ticker => $data) {
    $totalStockValue += $data[3];
}
$totalNetWorth = $cash + $totalStockValue;
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
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
    </div>
<?php endif; ?>

<!-- Portfolio -->
<div class="form-container">
    <h3>Your Portfolio</h3>
    <table>
        <thead>
            <tr>
                <th>Ticker</th>
                <th>Shares</th>
                <th>Previous Price</th>
                <th>Current Price</th>
                <th>Total Value</th>
                <th>Gain/Loss</th>
            </tr>
        </thead>
        <tbody>
            <?php foreach ($portfolio as $ticker => $data): ?>
                <?php 
                    $gainLoss = ($data[2] - $data[1]) * $data[0]; 
                    $gainLossClass = $gainLoss >= 0 ? 'style="color:green"' : 'style="color:red"';
                ?>
                <tr>
                    <td><?= htmlspecialchars($ticker) ?></td>
                    <td><?= $data[0] ?></td>
                    <td>$<?= number_format($data[1], 2) ?></td>
                    <td>$<?= number_format($data[2], 2) ?></td>
                    <td>$<?= number_format($data[3], 2) ?></td>
                    <td <?= $gainLossClass ?>>$<?= number_format($gainLoss, 2) ?></td>
                </tr>
            <?php endforeach; ?>
        </tbody>
    </table>
    <p><strong>Cash:</strong> $<?= number_format($cash, 2) ?></p>
    <p><strong>Net Worth:</strong> $<?= number_format($totalNetWorth, 2) ?></p>
</div>

<!-- Stock Lookup -->
<div class="form-container">
    <h3>Lookup Stock Price</h3>
    <form action="" method="POST">
        <input type="text" name="ticker" placeholder="Stock Ticker" required>
        <button type="submit" name="lookup">Check Price</button>
    </form>
</div>

<!-- Buy/Sell Stock Form -->
<div class="form-container">
    <h3>Buy/Sell Stocks</h3>
    <form action="" method="POST">
        <input type="text" name="ticker" placeholder="Stock Ticker" required>
        <input type="number" name="shares" placeholder="Shares" required>
        <button type="submit" name="buy">Buy</button>
        <button type="submit" name="sell">Sell</button>
    </form>
</div>

<!-- Save/Load Portfolio -->
<div class="form-container">
    <h3>Save/Load Portfolio</h3>
    <form action="" method="POST">
        <input type="text" name="load_file" placeholder="Save File to Load" required>
        <button type="submit" name="load">Load Portfolio</button>
    </form>
    <form action="" method="POST">
        <input type="text" name="save_file_name" placeholder="Save File Name" required>
        <button type="submit" name="exit">Exit and Save</button>
    </form>
</div>

</div>

</body>
</html>
