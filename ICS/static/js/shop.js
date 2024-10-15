var quantities = {
    clothes1: 0,
    clothes2: 0,
    clothes3: 0,
    clothes4: 0,
    clothes5: 0,
    clothes6: 0,
    furniture1: 0,
    furniture2: 0,
    furniture3: 0,
    furniture4: 0,
    furniture5: 0,
    furniture6: 0,
    device1: 0,
    device2: 0,
    device3: 0,
};

function increaseQuantity(clothesId) {
    quantities[clothesId]++;
    document.getElementById(clothesId + '-quantity').textContent = quantities[clothesId];
    calculateTotal();
}

function decreaseQuantity(clothesId) {
    if (quantities[clothesId] > 0) {
        quantities[clothesId]--;
        document.getElementById(clothesId + '-quantity').textContent = quantities[clothesId];
        calculateTotal();
    }
}

function calculateTotal() {
    var total = 0;
    var discount = 0;
    document.querySelectorAll('.menu-item').forEach(function (menuItem) {
        var price = parseFloat(menuItem.dataset.price);
        var id = menuItem.querySelector('.quantity-controls span').id.split('-')[0];
        total += quantities[id] * price;
    });

    if (total >= 100) {
        discount = Math.floor(total / 100) * 10;
    }
    var finalTotal = total - discount;

    document.getElementById('id-total-price').textContent = '¥' + total.toFixed(2);
    document.getElementById('id-discount').textContent = '¥' + discount.toFixed(2);
    document.getElementById('id-final-price').textContent = '¥' + finalTotal.toFixed(2);

    var placeOrderButton = document.getElementById('lang-ORDER_BUTTON');
    placeOrderButton.disabled = finalTotal === 0;
}
function placeOrder() {
    if (confirm('确认要下单吗？')) {
        var orderData = [];
        document.querySelectorAll('.menu-item').forEach(function (menuItem) {
            var price = parseFloat(menuItem.dataset.price);
            var id = menuItem.querySelector('.quantity-controls span').id.split('-')[0];
            var quantity = quantities[id];
            if (quantity > 0) {
                var name = menuItem.querySelector('h4').textContent;
                orderData.push({ id, name, price, quantity });
            }
        });

        localStorage.setItem('orderData', JSON.stringify(orderData));
        alert('订单已提交！');
        window.location.href = '/cart';  // 修改为你的实际页面地址
    } else {
        return;
    }
}
// // 检查登录状态并更新导航栏
// document.addEventListener('DOMContentLoaded', function () {
//     if (localStorage.getItem('loggedIn') === 'true') {
//         document.getElementById('loginLink').innerText = '登出';
//         document.getElementById('loginLink').href = '#';
//         document.getElementById('loginLink').onclick = function () {
//             localStorage.removeItem('loggedIn');
//             window.location.href = 'HOME.html';
//         };
//     }
// });

function placeOrder() {
    if (confirm('确认要下单吗？')) {
        var orderData = [];
        document.querySelectorAll('.menu-item').forEach(function (menuItem) {
            var price = parseFloat(menuItem.dataset.price);
            var id = menuItem.querySelector('.quantity-controls span').id.split('-')[0];
            var quantity = quantities[id];
            if (quantity > 0) {
                var name = menuItem.querySelector('h4').textContent;
                orderData.push({ id, name, price, quantity });
            }
        });

        localStorage.setItem('orderData', JSON.stringify(orderData));
        alert('订单已提交！');
        window.location.href = '/cart';  // 修改为你的实际页面地址
    } else {
        return;
    }
}

calculateTotal();
