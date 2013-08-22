(function($){
  $(document).ready(function() {
    $('#DinamicSolicitant .drow td').find('input[type!="checkbox"]').val('');

    $('#doc_mandatar_type_procura_2').bind('change', function(){
      $('.addit-mandatar_type').show();
      $('.addit-mandatar_type input').addClass('required');
    });

    if($('#doc_mandatar_type_procura_1').attr('checked')) {
      $('.addit-mandatar_type').hide();
      $('.addit-mandatar_type input').removeClass('required');
    }
    else {
      $('.addit-mandatar_type input').addClass('required');
    }

    $('#doc_mandatar_type_procura_1').bind('change', function(){
      $('.addit-mandatar_type').hide();
      $('.addit-mandatar_type input').removeClass('required');
    });

    $('#doc_declaram_este_originala_2').bind('click', function(){
      $('.addit-partial_forma').show();
      $('.addit-partial_forma input').addClass('required');
    });

    $('#doc_declaram_este_originala_1').bind('click', function(){
      $('.addit-partial_forma').hide();
      $('.addit-partial_forma input').removeClass('required');
    });

    if($('#doc_declaram_este_originala_1').attr('checked')) {
      $('.addit-partial_forma').hide();
      $('.addit-partial_forma input').removeClass('required');
    }
    else {
      $('.addit-partial_forma input').addClass('required');
    }

    $('#doc_solicit_data_incheiat_enable').bind('click', function(){
      if($(this).attr('checked') == true) {
        $('#doc_solicit_data_incheiat').removeAttr('disabled');
        $('#doc_solicit_data_incheiat').addClass('required');
        add_required_field_image($('#doc_solicit_data_incheiat'));
      }
      else {
        $('#doc_solicit_data_incheiat').attr('disabled', 'disabled');
        $('#doc_solicit_data_incheiat').removeClass('required');
        remove_required_field_image($('#doc_solicit_data_incheiat'));
      }
    });

    if($('#doc_solicit_data_incheiat_enable').attr('checked') == true) {
      $('#doc_solicit_data_incheiat').removeAttr('disabled');
      $('#doc_solicit_data_incheiat').addClass('required');
      add_required_field_image($('#doc_solicit_data_incheiat'));
    }
    else {
      $('#doc_solicit_data_incheiat').attr('disabled', 'disabled');
      $('#doc_solicit_data_incheiat').removeClass('required');
      remove_required_field_image($('#doc_solicit_data_incheiat'));
    }

    $('#mywebform-edit-form').bind('submit', function(){
      $('.fiz_choise').each(function(){
        if($(this).attr('checked') == true) {
          $(this).parent().siblings('.juridic').find('input[type!="checkbox"]').val('');
        }
        else {
          $(this).parent().siblings('.phisic').find('input[type!="checkbox"]').val('');
        }
      });

      $('.country-code').each(function(){
        var temp_parent = $(this).parent().parent();
        if($(this).val() == 'MD') {
          temp_parent.find('.not_in_md input, .not_in_md select').val('');
        }
        else {
          temp_parent.find('.in_md input, .in_md select').find('input').val('');
        }
      });

      if($('#doc_mandatar_type_procura_2').attr('checked') != true) {
        $('#doc_mandatar_nr_procura').val('');
        $('#doc_mandatar_data_procura').val('');
      }

      if($('#doc_declaram_este_originala_2').attr('checked') != true) {
        $('#doc_declaram_partial_forma').val('');
      }

      if($('#doc_creator_identic_solicitant_2').attr('checked') != true) {
        $('#DinamicCreator').remove();
      }

      if($('#doc_solicit_data_incheiat_enable').attr('checked') != true) {
        $('#doc_solicit_data_incheiat').val('');
      }

      return true;
    });

  });

  $('.jur_choise').live('click', function(){
    $(this).parent().siblings('.phisic').hide();
    $(this).parent().siblings('.juridic').show();
    $(this).parent().siblings('.juridic').find('input').addClass('required');
    $(this).parent().siblings('.phisic').find('input').removeClass('required');
  });

  $('.fiz_choise').live('click', function(){
    $(this).parent().siblings('.juridic').hide();
    $(this).parent().siblings('.phisic').show();
    $(this).parent().siblings('.phisic').find('input').addClass('required');
    $(this).parent().siblings('.juridic').find('input').removeClass('required');
  });

  $('.country-code').live('change', function() {
    var temp_parent = $(this).parent().parent();
    if($(this).val() == 'MD') {
      temp_parent.find('.not_in_md').hide();
      temp_parent.find('.in_md').show();
      temp_parent.find('.in_md input, .in_md select').addClass('required');
      temp_parent.find('.not_in_md input, .not_in_md select').removeClass('required');
    }
    else {
      temp_parent.find('.in_md').hide();
      temp_parent.find('.not_in_md').show();
      temp_parent.find('.in_md input, .in_md input select').removeClass('required');
      temp_parent.find('.not_in_md input, .not_in_md select').addClass('required');
    }
  });

  $('#doc_creator_identic_solicitant_2').live('click', function(){
    $('#DinamicCreator').show();
    $('#DinamicCreator').find('input, select').addClass('required');
  });

  $('#doc_creator_identic_solicitant_1').live('click', function(){
    $('#DinamicCreator').hide();
    $('#DinamicCreator').find('input, select').removeClass('required');
  });

  $('.document_type').live('change', function(){
    if($(this).val() == '1') {
      $(this).parent().parent().find('.document_another_type').show();
      $(this).parent().parent().find('.document_another_type').find('input').addClass('required');
    }
    else {
      $(this).parent().parent().find('.document_another_type').hide();
      $(this).parent().parent().find('.document_another_type').find('input').removeClass('required');
    }
  });

  $('#mywebform-content').live('document_load', function(){
    $('.not_in_md').hide();
    $('.in_md').hide();
    $('.country-code').each(function(){
      var temp_parent = $(this).parent().parent();
      if($(this).val() == 'MD') {
        temp_parent.find('.in_md').show();
        temp_parent.find('.in_md input, .in_md select').addClass('required');
        temp_parent.find('.not_in_md input, .not_in_md select').removeClass('required');
      }
      else {
        temp_parent.find('.not_in_md').show();
        temp_parent.find('.not_in_md input, .not_in_md select').addClass('required');
        temp_parent.find('.in_md input, .in_md select').removeClass('required');
      }
    });

    $('.jur_choise').each(function(){
      if($(this).attr('checked')) {
        $(this).parent().siblings('.phisic').hide();
        $(this).parent().siblings('.juridic').show();
        $(this).parent().siblings('.juridic').find('input').addClass('required');
        $(this).parent().siblings('.phisic').find('input').removeClass('required');
      }
    });

    $('.fiz_choise').each(function(){
      if($(this).attr('checked')) {
        $(this).parent().siblings('.phisic').show();
        $(this).parent().siblings('.juridic').hide();
        $(this).parent().siblings('.phisic').find('input').addClass('required');
        $(this).parent().siblings('.juridic').find('input').removeClass('required');
      }
    });

    if($('#doc_creator_identic_solicitant_1').attr('checked')) {
      $('#DinamicCreator').hide();
      $('#DinamicCreator').find('input, select').removeClass('required');
    }
    else {
      $('#DinamicCreator').find('input, select').addClass('required');
    }

    $('.document_type').each(function(){
      if($(this).val() == '1') {
        $(this).parent().parent().find('.document_another_type').show();
        $(this).parent().parent().find('.document_another_type').find('input').addClass('required');
      }
      else {
        $(this).parent().parent().find('.document_another_type').hide();
        $(this).parent().parent().find('.document_another_type').find('input').removeClass('required');
      }
    });
  });

})(jQuery);
