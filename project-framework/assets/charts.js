// assets/charts.js
(function () {
  var style = getComputedStyle(document.documentElement);
  var accent = style.getPropertyValue('--accent').trim();
  var accent2 = style.getPropertyValue('--accent2').trim();
  var ink = style.getPropertyValue('--ink').trim();
  var muted = style.getPropertyValue('--muted').trim();
  var rule = style.getPropertyValue('--rule').trim();
  var bg2 = style.getPropertyValue('--bg2').trim();

  var el = document.getElementById('chart-hours');
  if (!el || typeof echarts === 'undefined') return;

  var chart = echarts.init(el, null, { renderer: 'svg' });

  var weeks = ['W1 从站', 'W2 主站', 'W3 RTOS', 'W4 调度镜像', 'W5 Linux', 'W6 联调'];
  var hours = [20, 18, 15, 18, 20, 15];
  var cumulative = [];
  hours.reduce(function (acc, v, i) { cumulative[i] = acc + v; return acc + v; }, 0);

  chart.setOption({
    animation: false,
    grid: { left: 8, right: 8, top: 28, bottom: 8, containLabel: true },
    tooltip: {
      trigger: 'axis',
      appendToBody: true,
      axisPointer: { type: 'shadow' },
      backgroundColor: bg2,
      borderColor: rule,
      textStyle: { color: ink, fontSize: 12 },
      formatter: function (params) {
        var i = params[0].dataIndex;
        var lines = ['<b>' + weeks[i] + '</b>', '预算 ' + hours[i] + ' h'];
        lines.push('累计 ' + cumulative[i] + ' h');
        return lines.join('<br/>');
      }
    },
    xAxis: {
      type: 'category',
      data: weeks,
      axisLine: { lineStyle: { color: rule } },
      axisTick: { show: false },
      axisLabel: { color: muted, fontSize: 11 }
    },
    yAxis: [
      {
        type: 'value',
        name: '小时',
        nameTextStyle: { color: muted, fontSize: 11 },
        splitLine: { lineStyle: { color: rule, type: 'dashed' } },
        axisLabel: { color: muted, fontSize: 11 }
      },
      {
        type: 'value',
        name: '累计',
        max: 120,
        nameTextStyle: { color: muted, fontSize: 11 },
        splitLine: { show: false },
        axisLabel: { color: muted, fontSize: 11 }
      }
    ],
    series: [
      {
        name: '每周预算',
        type: 'bar',
        barWidth: '46%',
        data: hours,
        itemStyle: { color: accent, borderRadius: [4, 4, 0, 0] },
        label: {
          show: true,
          position: 'top',
          color: ink,
          fontSize: 11,
          fontFamily: 'GeistMono, monospace'
        }
      },
      {
        name: '累计',
        type: 'line',
        yAxisIndex: 1,
        data: cumulative,
        smooth: true,
        symbol: 'circle',
        symbolSize: 6,
        lineStyle: { color: accent2, width: 2 },
        itemStyle: { color: accent2 },
        label: {
          show: true,
          position: 'right',
          color: accent2,
          fontSize: 10,
          fontFamily: 'GeistMono, monospace',
          formatter: '{c}h'
        }
      }
    ]
  });

  window.addEventListener('resize', function () { chart.resize(); });
})();
