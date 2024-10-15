let navLangObj = {
  'zh': {
    RECORD: '记录',
    HOME: '主页',
    SHOP: '商店',
    CART: '购物车',
    ORDER: '订单',
    SIZE: '字体大小',
    COLOR: '字体颜色',
    BTN_COLOR: '按钮颜色',
    BTN_FONT_COLOR: '按钮字体颜色'
  },
  'en': {
    RECORD: 'Record',
    HOME: 'home',
    SHOP: 'shop',
    CART: 'cart',
    ORDER: 'order',
    SIZE: 'Font Size',
    COLOR: 'Font Color',
    BTN_COLOR: 'Button Color',
    BTN_FONT_COLOR: 'Button Font Color'
  }
}

function toPage(path) {
  window.location.href = path;
}

function showOption() {
  document.querySelector('.lang').classList.remove('show')
  document.querySelector('.options').classList
    .toggle('show')
}

function showLang() {
  document.querySelector('.options').classList.remove('show')
  document.querySelector('.lang').classList
    .toggle('show')
}

async function logout() {
  let res = await fetch('/user/logout', {
    method: 'GET',
    headers: {
      'Content-Type': 'application/json'
    }
  })
  if (res.ok) {
    localStorage.removeItem('user_data')
    alert('退出成功')
    window.location.href = '/user/login'
  }

}

function initOption() {
  //判断localstorage中有没有名为 font_size 的数据，如果没有就添加，值为14
  if (!localStorage.getItem('font_size')) {
    localStorage.setItem('font_size', 14)
  }
  document.querySelector('#size-option').value = localStorage.getItem('font_size')
  //判断localstorage中有没有名为 font_color 的数据，如果没有就添加，值为#000000
  if (!localStorage.getItem('font_color')) {
    localStorage.setItem('font_color', '#000000')
  }
  document.querySelector('#color-option').value = localStorage.getItem('font_color')
  //判断localstorage中有没有名为 btn_color 的数据，如果没有就添加，值为#4b5ce3

  if (!localStorage.getItem('btn_color')) {
    localStorage.setItem('btn_color', '#4b5ce3')
  }
  document.querySelector('#btn-color-option').value = localStorage.getItem('btn_color')

  //判断localstorage中有没有名为 btn_font_color 的数据，如果没有就添加，值为#ffffff
  if (!localStorage.getItem('btn_font_color')) {
    localStorage.setItem('btn_font_color', '#ffffff')
  }
  document.querySelector('#btn-font-color-option').value = localStorage.getItem('btn_font_color')
}

function initUser() {
  let headerUser = document.querySelector('.header-user')
  let loginView = document.querySelector('.login-view')
  console.log(localStorage.getItem('user_data'));
  if (localStorage.getItem('user_data')) {
    headerUser.style.display = 'flex'
    loginView.style.display = 'none'
    let user = JSON.parse(localStorage.getItem('user_data'))
    document.getElementById('username').innerText = user.username
  } else {
    headerUser.style.display = 'none'
    loginView.style.display = 'flex'
  }
}

document.getElementById('size-option').addEventListener('input', function (e) {
  localStorage.setItem('font_size', e.target.value)
  //刷新当前页面
  window.location.reload()
})

document.getElementById('color-option').addEventListener('input', function (e) {
  localStorage.setItem('font_color', e.target.value)
  //刷新当前页面
  window.location.reload()
})

document.getElementById('btn-color-option').addEventListener('input', function (e) {
  localStorage.setItem('btn_color', e.target.value)
  //刷新当前页面
  window.location.reload()
})

document.getElementById('btn-font-color-option').addEventListener('input', function (e) {
  localStorage.setItem('btn_font_color', e.target.value)
  //刷新当前页面
  window.location.reload()
})

function initLanguage() {
  let lang = localStorage.getItem('lang');
  if (!lang) {
    localStorage.setItem('lang', 'zh')
  }
}


function changeLanguage(lang, langObj) {
  localStorage.setItem('lang', lang);
  langChange(navLangObj)
  langChange(langObj)
}

function langChange(pageLangObj) {
  let lang = localStorage.getItem('lang') || 'zh';
  if (lang) {
    let obj = pageLangObj[lang];
    for (let key in obj) {
      document.querySelector(`#lang-${key}`).textContent = obj[key];
    }
  }
}


initLanguage()
langChange(navLangObj)
initUser()
initOption()

