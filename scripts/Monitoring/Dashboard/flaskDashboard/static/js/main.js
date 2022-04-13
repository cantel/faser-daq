const timezone = window.dayjs_plugin_timezone;
const UTC = window.dayjs_plugin_utc;
dayjs.extend(timezone);
dayjs.extend(UTC);

const EventBus = new Vue();
var app = new Vue({
  el: "#app",
  vuetify: new Vuetify(),
  data: {
    c_tags: [],
    c_ids: [],
    c_safeMode: true,
    c_redis_infos: null,
    c_data: {},
    c_eventSource: null,
    c_busy: false,
    c_currentPage: 1,
  },
  delimiters: ["[[", "]]"],

  methods: {
    getIDs(selected_tags) {
      this.c_tags = selected_tags;
      this.c_total_rendered = 0;
      this.c_ready = false;

      axios
        .post("/IDs_from_tags", {
          tags: selected_tags,
        })
        .then((response) => {
          this.c_ids = response.data;
          // this.c_ids = response.data.sort().concat(response.data.sort()).concat(response.data.sort()).concat(response.data.sort()).concat(response.data.sort()).concat(response.data.sort()).concat(response.data.sort()).concat(response.data.sort()).concat(response.data.sort()).concat(response.data.sort()).concat(response.data.sort()).concat(response.data.sort()).concat(response.data.sort()).concat(response.data.sort()).concat(response.data.sort()).concat(response.data.sort());
          setTimeout(() => {
            this.createEventSource(this.hist_currentPage);
          }, 3000);
        });
    },
    async createEventSource(ids) {
      // console.log("EventSource created")
      if (this.c_eventSource) {
        this.c_eventSource.close();
      }
      this.c_eventSource = new EventSource(
        `/stream_histograms/${ids.join("&")}`
      );
      this.c_eventSource.onmessage = async (event) => {
        // console.log(event.data)
        // this.c_data = JSON.parse(event.data)
        await this.handleData(event.data);
      };
    },
    handleData(data) {
      return new Promise((resolve, reject) => {
        this.c_data = JSON.parse(data);
        resolve();
      });
    },

    deleteHistory() {
      axios.get("/delete_history");
    },

    deleteTags() {
      axios.get("/delete_tags").then((response) => {
        window.location.reload(true);
      });
    },
    deleteCurrent() {
      axios.get("/delete_current").then((response) => {
        window.location.reload(true);
      });
    },
    download_plots() {
      this.c_busy = true;
    
      axios
        .post(
          "/download_plots",
          {
            IDs: this.c_ids[this.c_currentPage-1],
          },
          { responseType: "blob" }
        )
        .then((response) => {
          var fileURL = window.URL.createObjectURL(new Blob([response.data]));
          var fURL = document.createElement("a");

          fURL.href = fileURL;
          fURL.setAttribute("download", "plots.zip");
          document.body.appendChild(fURL);
          fURL.click();
          this.c_busy = false;
        });
    },
  },
  computed: {
    hist_currentPage: function () {
      return this.c_ids[this.c_currentPage - 1];
    },
  },
  watch: {
    c_currentPage: {
      handler: function () {
        setTimeout(() => {
          this.createEventSource(this.hist_currentPage);
        }, 2000);
      },
    },
  },
});
