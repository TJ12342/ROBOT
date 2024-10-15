$(function () {
  let url = "http://123.57.208.175:8081/face/list"
  new ProductTableApp(
    $('#product-table-app'),
    url,
  );
});

class ProductTableApp {
  /**
   * @param {JQuery<HTMLElement>} $el
   * @param {string} url
   */
  constructor($el, url) {
    this.initState();
    this.initOption();
    this.fetchJson(url, $el);
  }

  initState() {
    this.state = { isLoaded: false, products: [], err: null, fontColor: null, fontSize: null };
  }

  initOption() {
    this.state.fontSize = localStorage.getItem('font_size') || 14
    this.state.fontColor = localStorage.getItem('font_color') || '#000000'
  }

  /**
   * @param {string} url
   * @returns {Array}
   */
  async fetchJson(url, $el) {
    let res = await fetch(url, {
      method: "GET",
      headers: { "Content-Type": "application/json" },
    })
    if (res.ok) {
      let resData = await res.json()
      this.state.isLoaded = true;
      this.state.products = resData.data;
      console.log(this.state.products);
      this.defineElements($el, this.state.products);
      this.render(this.state.products);
      this.bindEvents();
    }
  }

  /**
   *
   * @param {JQuery<HTMLElement>} $el
   * @param {Array} products
   */
  defineElements($el, products) {
    const users = [...new Set(products.map((product) => product.name))];

    this.$el = $el;
    this.$tbody = this.$el.find('tbody');
    this.$noResults = this.$el.find('#no-results');
    this.$handleTable = this.$el.find('.js-handle-table');
    this.$sortBy = this.$el.find('#sort-by');
    this.$filter = this.$el.find('.js-filter');
    this.$filterUser = this.$el.find('#filter-User')
    if (users.length > 0) this.$filterUser.append(users.map((item) => new Option(item, item)))
  }

  bindEvents() {
    this.handleChange = this.handleChange.bind(this);
    this.$handleTable.on('change', this.handleChange);
  }

  handleChange() {
    const sorted = this.sort(this.state.products);
    const filtered = this.filter(sorted);
    // const toggled = this.toggle(filtered);
    this.render(filtered);
  }

  /**
   * @param {Array} products
   * @returns {Array}
   */
  sort(products) {
    const $selectedSortTarget = this.$sortBy.find('option:selected');
    const val = $selectedSortTarget.val();


    if (val === 'none') {
      return products;
    }


    if (val === 'created_at')
      return [...products].sort((a, b) => {

        const toDate = (dateString) => {
          const momentObject = moment(dateString);
          const dateObject = momentObject.toDate();
          return dateObject;
        };

        if (toDate(a[val]) < toDate(b[val])) return -1;
        if (toDate(a[val]) > toDate(b[val])) return 1;
        return 0;
      });
  }

  /**
   * @param {Array} products
   * @returns {Array}
   */
  filter(products) {
    const $selectedUser = this.$filterUser.find('option:selected');


    /**
     * @param {Object} product
     */
    const isUserValid = (product) => {
      return $selectedUser.val() === 'all'
        ? product
        : product.name === $selectedUser.text();
    };




    return products.filter(isUserValid);
  }

  /**
   * @param {Array} products
   * @returns {Array}
   */


  render(products) {
    const twoSpace = '&nbsp;&nbsp;';

    if (!this.state.isLoaded) {
      this.$tbody.html('<div>Loading...</div>');
      return;
    }

    this.$tbody.html(
      products.map(
        (product) =>
          `<tr class="table-row" data-key="${product.id}">
            <td class="table-cell align-left">${product.id}</td>
           
            <td class="table-cell align-left" style="font-size: ${this.state.fontSize}px;color: ${this.state.fontColor}">${product.name}</td>
            <td class="table-cell align-left" style="font-size: ${this.state.fontSize}px;color: ${this.state.fontColor}">${product.gender}</td>
            <td class="table-cell align-left" style="font-size: ${this.state.fontSize}px;color: ${this.state.fontColor}">${product.department}</td>
            <td class="table-cell align-left" style="font-size: ${this.state.fontSize}px;color: ${this.state.fontColor}">${product.major}</td>
            <td class="table-cell align-left" style="font-size: ${this.state.fontSize}px;color: ${this.state.fontColor}">${product.class_}</td>
            <td class="table-cell align-left">${this.loadBase64Img(product.features)}</td>
            <td class="table-cell align-right" style="font-size: ${this.state.fontSize}px;color: ${this.state.fontColor}">${moment(product.created_at).format('YYYY/MM/DD HH:mm:ss')}</td>
            
          </tr>`,
      ),
    );

    products.length === 0
      ? this.$noResults.removeClass('hidden')
      : this.$noResults.addClass('hidden');
  }

  loadBase64Img(base64String) {
    //返回一个img标签，标签使用base64String作为图片
    return `<img style="width: 48px; height: 48px;object-fit: cover;" src="data:image/png;base64,${base64String}" alt="暂无图片" />`;
  }
}


