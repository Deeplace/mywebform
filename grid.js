var jscal;
var grids = {};
var grid_total_rows = 0;
var grid_loaded_rows = 0;
var global_offset = 10;
var global_timeout = 50;
var grid_data_hook = {};

(function ($) {
Drupal.behaviors.grid = {
  attach : function (context) {
    $.each(Drupal.settings.mywebform.grids, function(gridname, cnt) {
      grids[gridname] = { 'count' : cnt, 'start' : 0, 'offset' : global_offset, 'maxindex': 0};
      grid_total_rows += cnt;
    });

    if(!Drupal.settings.mywebform.preview) {
      jscal = Calendar.setup({onSelect: function(jscal) {jscal.hide()}, dateFormat : "%d.%m.%Y"});

      $('a.grid-addrow').live('click', function() {
        if(!$(this).hasClass('disabled')) {
          var gridname = $(this).parent().attr('id');
          grid_add_row(gridname, 0);
        }
        return false;
      }).trigger('click');

      $('td.delrow a').live('click', function(evt) {
        evt.preventDefault();
        grid_remove_row($(this));
      });
    }
    if(!Drupal.settings.mywebform.print) {
      grid_load_data();
    }
    else {
      //window.print();
    }
  }
};

// Code to execute when webform load is finished
function webform_loaded() {
  $('a.grid-addrow').removeClass('disabled');

  $.each(webform.afterLoad, function() {
    this();
  });

  if(Drupal.settings.mywebform.print) {
    //window.print();
  }

  $('#mywebform-content').trigger('document_load');
}

function grid_load_data() {
  if(grid_total_rows) {
    progress_show();
    $('a.grid-addrow').addClass('disabled');
    grid_get_data();
  } else {
    webform_loaded();
  }
}

function grid_get_data() {
  var gridName = '';
  var gridStart = 0;
  var gridOffset = 0;

  // Find a grid that needs to be loaded
  $.each(grids, function (index, value) {
    if(value['start'] < value['count']) {
      gridName = index;
      gridStart = value['start'];
      gridOffset = value['offset'];
      return false;
    }
  });

  if(Drupal.settings.mywebform.load_url == '') gridName = '';

  // Return if no more grids needs loading
  if(gridName == '') {
    progress_hide();
    webform_loaded();
    return false;
  }

  // Load a specific grid data
  $.getJSON(Drupal.settings.mywebform.load_url, {'grid' : gridName, 'start' : gridStart, 'offset' : gridOffset}, function(data) {
    // data['grids'] - array of row indices
    // data['grid'] - grid's name
    // data['start'] - start parameter
    // data['offset'] - offset parameter
    // data['fields'] - fields' values

    // Add rows
    $.each(data['grids'], function(i, index) {
      grid_add_row(data['grid'], index);
      grid_loaded_rows++;
      progress_set(parseInt(100 * grid_loaded_rows / grid_total_rows));
    });

    $.each(grid_data_hook, function() {
      data = this(data);
    });

    // Fill added rows with values
    if(Drupal.settings.mywebform.preview) {
      $.each(data['fields'], function(id, field) {
        if(field.type == 'file') {
          $('#' + id).html(field.info.filelink);
        }
        else if((field.type == 'select') && (field.info == 'document_type')) {
            $('#' + id).val(field.value);
        }
        else {
          $('#' + id).html(field.value);
        }
      });
    } else {
      $.each(data['fields'], function(id, field) {
        if(field.type == 'select') {
          if(field.container.length > 0) {
            $('#' + id).empty().append($('<option></option>').attr('value', field.value).text(field.content));
          } else if($('#' + id + ' option').size() <= 1 && $('#' + id).val() == '') {
            $('#' + id).empty().append($('<option></option>').attr('value', field.value).text(field.value));
          } else {
            $('#' + id).val(field.value);
          }
          if(typeof jQuery.uniform.update == 'function') {
            jQuery.uniform.update('#' + id);
          }
        } else if(field.type == 'file') {
          $('#' + id).val(field.value);
          if(field.value != '') {
            $('#' + id).parent().children('div.file-info').each(function() {
              $(this).show();
              $(this).children('.file').addClass(field.info.ext);
              $(this).children('.filename').html(field.info.filelink);
              $(this).children('.filesize').html(field.info.size);
            });
          }
        }
        else if(field.type == 'checkbox'){
          if(field.value != '') {
            $('#' + id).trigger('click').attr('checked','checked');
          }
        }
        else {
          $('#' + id).val(field.value);
        }
      });
    }

    // Update offset
    grids[data['grid']]['start'] += parseInt(data['offset']);

    // Start function after some time again
    setTimeout(grid_get_data, global_timeout);
  });
}

function grid_add_row(gridname, index) {
  if(index <= 0) {
    index = grids[gridname]['maxindex'] + 1;
  }

  if(!$('#' + gridname + ' .row-' + index).length) {
    var drow = $('#' + gridname + ' .drow');
    var row = drow.clone();

    row.addClass('row-' + index);
    row.removeClass('drow');
    row.show();
    row.find('input, select, textarea, span.form-item').each(function (i) {
      if($(this).hasClass('row-index')) {
        $(this).val(index);
      }
      $(this).attr('row-index', index);
      $(this).trigger('change');
      var autofield_func = $(this).attr('autofield_func');
      if(autofield_func) {
        funcs = autofield_func.split(' ');
        for(var i in funcs) {
          $(this).change(window[funcs[i]]);
        }
      }
      $(this).attr('name', $(this).attr('name').replace('[0]', '') + '[' + index + ']');
      $(this).attr('id', $(this).attr('id').replace('-0', '') + '-' + index);
      if(typeof jQuery.uniform == 'object') {
        if(jQuery(this).is('select')) {
          jQuery(this).uniform();
        }
      }
    });

    row.find('input:checkbox').each(function(){
        if(jQuery(this).attr('radio') == 'on') {
          if(jQuery(this).siblings('input[rel="' + jQuery(this).attr('rel') +'"]:checked').length == 0) {
            jQuery(this).trigger('click').attr('checked', 'checked');
          }
        }
      });

    /*row.find('td.delrow a').click(function(evt) {
      evt.preventDefault();
      grid_remove_row($(this));
    });*/

    // Insert row in table
    row.insertBefore('#' + gridname + ' .drow');
  }

  // Update max row index
  if(index > grids[gridname]['maxindex']) {
    grids[gridname]['maxindex'] = index;
  }

  // JSCAL
  $('#' + gridname + ' .row-' + index + ' input.date').each(function () {
    var id = $(this).attr('id');
    if(id && !Drupal.settings.mywebform.preview) {
      jscal.manageFields(id, id, "%d.%m.%Y");
    }
  });

}

function grid_remove_row(obj) {
  var gridname = obj.parent().parent().parent().parent().parent().attr('id');
  var maxindex = 0;

  // Do not delete the unique row
  if($('#' + gridname + ' tbody tr').length <= 2) return;

  // Recalculate autofields
  obj.parent().parent().find('input[type=text]').each(function() {
    var autofield_func = $(this).attr('autofield_func');
    if(autofield_func) {
      $(this).val('');
      $(this).trigger('change');
      /*funcs = autofield_func.split(' ');
      for(var i in funcs) {
        window[funcs[i]]($(this));
      }*/
    }
  });

  // Remove row
  obj.parent().parent().remove();

  // Recalculate maxindex
  $('#' + gridname + ' tbody tr').each(function (i) {
    if(!$(this).hasClass('drow')) {
      var index = parseInt($(this).attr('class').replace('row-', ''));
      if(!isNaN(index) && index >= maxindex) maxindex = index;
    }
  });
  grids[gridname]['maxindex'] = maxindex;
}

function progress_show() {
  $('.progressbar').show();
}

function progress_hide() {
  $('.progressbar').hide();
}

function progress_set(percent) {
  if(percent > 0) {
    $('.progressbar .current').css('width', percent + '%');
    $('.progressbar .current').show();
    $('.progressbar .current span').text(percent + ' %');
  } else {
    $('.progressbar .current').css('width', '0%');
    $('.progressbar .current').hide();
    $('.progressbar .current span').text('0 %');
  }
}

})(jQuery);
